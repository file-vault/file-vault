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

#include <pwd.h>
#include <fcntl.h>
#include <time.h>

#define TM_FMT "%Y-%m-%d %H:%M:%S"
#define NETLINK_TEST    30
#define MSG_LEN            125
#define MAX_PLOAD        125

typedef struct _user_msg_info
{
    struct nlmsghdr hdr;
    char  msg[MSG_LEN];
} user_msg_info;

//MYSQL mysql;
FILE *logfile;
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
/*
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
    }else {
        //printf("[%s]\n", query);
    }
    res = mysql_store_result(&mysql);
    //mysql_fetch_row检索结果集的下一行
    if (row = mysql_fetch_row(res)) {
        //mysql_num_fields返回结果集中的字段数目
        if (string_to_long_unsigned(row[2])==uid){
            return 1;
        }
        else return 0;
    }
    else
        return 2;
}
*/
unsigned get_unsigned_length(unsigned long num) {
    unsigned length = 0;
    while (num) {
        num /= 10;
        length++;
    }
    return length;
}

void Log(char *commandname, int uid, int pid, char *file_path, int flags, int ret)
{
	char logtime[64];
	char username[32];
	struct passwd *pwinfo;
	char openresult[10];
	char opentype[16];
	strcpy(openresult, "failed");
	if (flags & O_RDONLY) strcpy(opentype, "Read");
	else if (flags & O_WRONLY) strcpy(opentype, "Write");
	else if (flags & O_RDWR) strcpy(opentype, "Read/Write");
	else strcpy(opentype, "other");

	time_t t = time(0);
	if (logfile == NULL)	return;
	pwinfo = getpwuid(uid);
	strcpy(username, pwinfo->pw_name);

	strftime(logtime, sizeof(logtime), TM_FMT, localtime(&t));
	fprintf(logfile, "%s(%d) %s(%d) %s \"%s\" %s %s\n", username, uid, commandname, pid, logtime, file_path, opentype, openresult);
	fflush(logfile);
	printf("%s(%d) %s(%d) %s \"%s\" %s %s\n", username, uid, commandname, pid, logtime, file_path, opentype, openresult);
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
    saddr.nl_pid = 200;  //端口号(port ID)
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

    /*mysql_init(&mysql);
    if(!mysql_real_connect(&mysql, "localhost", "root", "123456", "test", 0, NULL, 0)) {
        printf("Failed to connect to Mysql!\n");
        return -1;
    }else {
        printf("Connected to Mysql successfully!\n");
    }*/

	logfile = fopen(logpath, "a+");  //打开文件
	if (logfile == NULL) {
		printf("Waring: can not create log file\n");
		exit(1);
	}
	else {
		printf("open log file successfully!");
	}

    sprintf(umsg,"daemon log start");
    memcpy(NLMSG_DATA(nlh), umsg, strlen(umsg));
    printf("%s\n",umsg);
    /*ret = sendto(skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
    if(!ret)
    {
        perror("send to error\n");
        close(skfd);
        exit(-1);
    }
	*/
    while(1)
    {
		unsigned int uid, pid, flags, ret;
		char * file_path;
		char * commandname;
        //recv from kernel of the ino and uid
        memset(&u_info, 0, sizeof(u_info));
        ret = recvfrom(skfd, &u_info, sizeof(user_msg_info), 0, (struct sockaddr *)&daddr, &len);
        if(!ret)
        {
            perror("recv form kernel error\n");
            close(skfd);
            exit(-1);
        }
       
		//判断消息的种类，日志信息的情况
			printf("log from kernel: %s\n", u_info.msg);

			uid = *((unsigned int *)u_info.msg);
			pid = *(1 + (int *)u_info.msg);
			flags = *(2 + (int *)u_info.msg);
			ret = *(3 + (int *)u_info.msg);
			commandname = (char *)(4 + (int *)u_info.msg);
			file_path = (char *)(4 + 16 / 4 + (int *)u_info.msg);
			Log(commandname, uid, pid, file_path, flags, ret);
			//fprintf(logfile, "log from kernel: %s\n", u_info.msg);
			/*char *q;
			q = strtok(u_info.msg, ":");
			char *path = q;
			q = strtok(NULL, ":");
			char *opt = q;
			q = strtok(NULL, "");
			unsigned int userid = string_to_long_unsigned(q);
			fprintf(logfile, "%s %s %d\n", path, opt, userid);*/		
    }
	if (logfile != NULL)
		fclose(logfile);
    close(skfd);
    free((void *)nlh);
    return 0;
}
