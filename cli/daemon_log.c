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


FILE *logfile;

void Log(char *commandname, int uid, int pid, char *file_path, int flags, int ret)
{
	char logtime[64];
	char username[32];
	struct passwd *pwinfo;
	char openresult[10];
	//char opentype[16];
	strcpy(openresult, "denied");
	
	/*if (flags & O_RDONLY) strcpy(opentype, "Read");
	else if (flags & O_WRONLY) strcpy(opentype, "Write");
	else if (flags & O_RDWR) strcpy(opentype, "Read/Write");
	else strcpy(opentype, "other");*/

	time_t t = time(0);
	if (logfile == NULL)	return;
	pwinfo = getpwuid(uid);
	strcpy(username, pwinfo->pw_name);

	strftime(logtime, sizeof(logtime), TM_FMT, localtime(&t));
	//fprintf(logfile, "%s(%d) %s(%d) %s \"%s\" %s %s\n", username, uid, commandname, pid, logtime, file_path, opentype, openresult);
	fprintf(logfile, "%s(%d) %s(%d) %s path:\"%s\" %s \n", username, uid, commandname, pid, logtime, file_path, openresult);
	fflush(logfile);
	//printf("%s(%d) %s(%d) %s \"%s\" %s %s\n", username, uid, commandname, pid, logtime, file_path, opentype, openresult);
	printf("%s(%d) %s(%d) %s path:\"%s\" %s \n", username, uid, commandname, pid, logtime, file_path,  openresult);
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
       		
			printf("log from kernel: %s\n", u_info.msg);
			//解析记录消息
			uid = *((unsigned int *)u_info.msg);
			pid = *(1 + (int *)u_info.msg);
			flags = *(2 + (int *)u_info.msg);
			ret = *(3 + (int *)u_info.msg);
			commandname = (char *)(4 + (int *)u_info.msg);
			file_path = (char *)(4 + 16 / 4 + (int *)u_info.msg);
			Log(commandname, uid, pid, file_path, flags, ret); //调用LOG函数进行记录
	
    }
	if (logfile != NULL)
		fclose(logfile);
    close(skfd);
    free((void *)nlh);
    return 0;
}
