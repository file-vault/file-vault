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
#include <time.h>

#define NETLINK_TEST    30
#define MSG_LEN            125
#define MAX_PLOAD        125
#define TM_FMT "%Y-%m-%d %H:%M:%S"

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

    // cur_uid / ino sent to be checked, return res
    // 1) if ino in uid's table check_res=1
    // 2) else if ino in other's table check_res=0
    // 3) else check_res=1
    char query[100];
    sprintf(query,"select * from t%d",uid);
    printf("query:%s\n",query);
    MYSQL_RES *res,*res2;
    MYSQL_ROW row,row2;
    int flag;
    flag = mysql_real_query(&mysql, query, (unsigned int)strlen(query));
    if(flag) {
        printf("Query failed!\n");
        return -1;
    }else {
        //printf("[%s]\n", query);
    }
    int flag2;
    flag2=0;
    /*mysql_store_result讲全部的查询结果读取到客户端*/
    res = mysql_store_result(&mysql);
    /*mysql_fetch_row检索结果集的下一行*/
    while(row = mysql_fetch_row(res)) {
        /*mysql_num_fields返回结果集中的字段数目*/
        //printf("this a res %s\n",row[1]);
        if (string_to_long_unsigned(row[1])==ino)
        {
            printf("this a res %u\n",string_to_long_unsigned(row[1]));
            printf("in it\n");
            flag2=1;
            break;
        }
    }
    if (flag2==0)
        printf("not in it\n");

    int flag3;
    char cur_table[10];
    flag3=0;
    if (flag2==1)
    {
        printf("%d\n",flag2);
        return flag2;
    }
    else
    {
        //check other table,if in other table,flag3=0
        sprintf(query,"show tables");
        flag = mysql_real_query(&mysql, query, (unsigned int)strlen(query));
        if(flag) {
            printf("Query failed!\n");
            return -1;
        }
        /*mysql_store_result讲全部的查询结果读取到客户端*/
        res = mysql_store_result(&mysql);
        /*mysql_fetch_row检索结果集的下一行*/
        flag3=0;
        while(row = mysql_fetch_row(res)) {
            sprintf(cur_table,"t%u",uid);
            if (strcmp(row[0],cur_table)==0)
                continue;
            sprintf(query,"select * from %s",row[0]);
            printf("[%s]\n",query);
            flag = mysql_real_query(&mysql, query, (unsigned int)strlen(query));
            if(flag) {
                printf("Query failed!\n");
                return -1;
            }
            res2 = mysql_store_result(&mysql);
            while(row2 = mysql_fetch_row(res2)) {
                //printf("this a res %s\n",row2[1]);
                if (string_to_long_unsigned(row2[1])==ino){
                        printf("this a res %u\n",string_to_long_unsigned(row2[1]));
                        printf("in other's\n");
                        flag3=1;
                        break;
                    }
            }
        }
        printf("%d\n",!flag3);
        if (flag3)
            return 0;
        else 
        {
            printf("not in other's\n");
            return 1;
        }
    }
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

	char logtime[64];
	time_t t;
	FILE *logfile;
	char logpath[32];
	if (argc == 1) strcpy(logpath, "./log");  //关于审计信息
	else if (argc == 2) strncpy(logpath, argv[1], 32);
	else {
		printf("commandline parameters error! please check and try it! \n");
		exit(1);
	}
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

	logfile = fopen(logpath, "w+");  //打开文件
	if (logfile == NULL) {
		printf("Waring: can not create log file\n");
		exit(1);
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
		if (strstr(u_info.msg, ":")) { //判断消息的种类，日志信息的情况
			printf("from kernel: %s\n", u_info.msg);
			char *q;
			q = strtok(u_info.msg, ":");
			char *path = q;
			q = strtok(NULL, ":");
			char *opt = q;
			q = strtok(NULL, "");
			unsigned int userid = string_to_long_unsigned(q);
			t = time(0);
			strftime(logtime, sizeof(logtime), TM_FMT, localtime(&t));
			fprintf(logfile, "%s %s %d %s\n", path, opt, userid,logtime);
		}
		else {
			printf("from kernel: %s\n", u_info.msg);
			char *p;
			p = strtok(u_info.msg, " ");
			unsigned int uid = string_to_long_unsigned(p);
			p = strtok(NULL, "");
			unsigned int ino = string_to_long_unsigned(p);

			result = my_query_privilege(ino, uid);
			//check mysql and send to netlink_test
			sprintf(umsg, "%d", result);
			memcpy(NLMSG_DATA(nlh), umsg, strlen(umsg));
			ret = sendto(skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
			if (!ret)
			{
				perror("send to error\n");
				close(skfd);
				exit(-1);
			}
		}
    }
	if (logfile != NULL)
		fclose(logfile);
    close(skfd);
    free((void *)nlh);
    return 0;
}
