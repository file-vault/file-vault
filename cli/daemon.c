#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <linux/netlink.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <mysql/mysql.h>
#include <sys/stat.h>
#include <ctype.h>

#define NETLINK_TEST    30
#define MSG_LEN            125
#define MAX_PLOAD        125

typedef struct _user_msg_info
{
    struct nlmsghdr hdr;
    char  msg[MSG_LEN];
} user_msg_info;

MYSQL mysql;

unsigned string_to_long_unsigned(char *s)
{
    long unsigned int result = 0;
    for (int i = strlen(s) - 1; i >= 0; --i){
        unsigned temp = 0;
        int k = strlen(s) - i - 1;
        if (isdigit(s[i])){
            temp = s[i] - '0';
            while (k--){
                temp *= 10;
            }
            result += temp;
        }
        else{
            break;
        }
    }
    return result;
}

int my_query_privilege(unsigned long ino,uid_t uid){
    int flag;
    char query[100];
    sprintf(query,"select * from info where ino=%lu",ino);
    MYSQL_RES *res;
    MYSQL_ROW row;
    flag = mysql_real_query(&mysql, query, (unsigned int)strlen(query));
    if(flag) {
        printf("Query failed!\n");
        return -1;
    }
    res = mysql_store_result(&mysql);
    /*mysql_fetch_row检索结果集的下一行*/
    if (row = mysql_fetch_row(res)) {
        /*mysql_num_fields返回结果集中的字段数目*/
        if (string_to_long_unsigned(row[1])==uid){
            return 1;
        }
        else return 0;
    }
    else
        return 1;
}

unsigned get_unsigned_length(unsigned long num) {
    unsigned length = 0;
    while (num) {
        num /= 10;
        length++;
    }
    return length;
}

int main(int argc, char **argv)
{
    int skfd;
    int ret;
    int result;
    user_msg_info u_info;
    socklen_t len;
    struct nlmsghdr *nlh = NULL;
    struct sockaddr_nl saddr, daddr;
    char umsg[200];
    unsigned short seq;

    /* 创建NETLINK socket */
    skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_TEST);
    if(skfd == -1)
    {
        perror("create socket error\n");
        return -1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.nl_family = AF_NETLINK; //AF_NETLINK
    saddr.nl_pid = 100;  //端口号(port ID)
    saddr.nl_groups = 0;

    if(bind(skfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0)
    {
        perror("bind() error\n");
        close(skfd);
        return -1;
    }

    memset(&daddr, 0, sizeof(daddr));
    daddr.nl_family = AF_NETLINK;
    daddr.nl_pid = 0; // to kernel
    daddr.nl_groups = 0;

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PLOAD));
    memset(nlh, 0, sizeof(struct nlmsghdr));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
    nlh->nlmsg_flags = 0;
    nlh->nlmsg_type = 0;
    nlh->nlmsg_seq = 0;
    nlh->nlmsg_pid = saddr.nl_pid; //self port

    mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, "localhost", "root", "123456", "filevault", 0, NULL, 0)) {
        printf("Failed to connect to Mysql!\n");
        return -1;
    }else {
        printf("Connected to Mysql successfully!\n");
    }

    // notify kernel daemon start to run
    sprintf(umsg,"daemon start");
    memcpy(NLMSG_DATA(nlh), umsg, strlen(umsg));
    printf("%s\n",umsg);
    ret = sendto(skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
    if(!ret)
    {
        perror("send to error\n");
        close(skfd);
        exit(-1);
    }

    while(1)
    {
        //recv from kernel of the ino and uid
        memset(&u_info, 0, sizeof(u_info));
        ret = recvfrom(skfd, &u_info, sizeof(user_msg_info), 0, (struct sockaddr *)&daddr, &len);
        if(!ret)
        {
            perror("recv form kernel error\n");
            close(skfd);
            exit(-1);
        }
        //parse the request
        char *p;
        p=strtok(u_info.msg," ");
        unsigned int uid=string_to_long_unsigned(p);
        p=strtok(NULL,"");
        unsigned int ino=string_to_long_unsigned(p);
        //check mysql according to the request 
        result = my_query_privilege(ino,uid); 
      	//send the result to the kernel
        sprintf(umsg,"%d",result);
        nlh->nlmsg_seq = u_info.hdr.nlmsg_seq;
        memcpy(NLMSG_DATA(nlh), umsg, strlen(umsg));
        ret = sendto(skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
        if(!ret)
        {
            perror("send to error\n");
            close(skfd);
            exit(-1);
        }
    }
    close(skfd);
    free((void *)nlh);
    return 0;
}
