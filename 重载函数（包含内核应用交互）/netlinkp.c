/****************************************
* Author: zhangwj
* Date: 2017-01-19
* Filename: netlink_test.c
* Descript: netlink of kernel
* Kernel: 3.10.0-327.22.2.el7.x86_64
* Warning:
******************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/ctype.h>
#define NETLINK_TEST     30
#define MSG_LEN          125
#define USER_PORT        100

struct sock *nlsk = NULL;
extern struct net init_net;
extern struct semaphore sem;
extern int check_result;
extern int daemon_flag;
static int ino_len = sizeof(unsigned long);
static atomic_t sequence = ATOMIC_INIT(0);

// 创建了一个队列，使用信号量保护
static struct queue {
    int data[65536];
    struct semaphore sem[65536];
} rspbuf;
DEFINE_RATELIMIT_STATE(rs, 3 * HZ, 1);

unsigned string_to_long_unsigned(char *s)
{
    int i;
    long unsigned int result = 0;
    for (i = strlen(s) - 1; i >= 0; --i){
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

int check_privilege(char *pbuf, uint16_t len)
{
    // sk_buff是内核态存储网络数据的重要结构
    struct sk_buff* skb;
    struct nlmsghdr* nlh;
    unsigned short seq;
    // 创建 sk_buff空间
    skb = nlmsg_new(ino_len, GFP_ATOMIC);
    if (!skb) {
        return 0;
    }
    // 设置netlink消息头部
    nlh = nlmsg_put(skb, 0, 0, NETLINK_TEST, len, 0);
    seq = atomic_inc_return(&sequence);
    nlh->nlmsg_seq = seq;
    //*(unsigned long*)NLMSG_DATA(nlh) = inode;
    memcpy(nlmsg_data(nlh), pbuf, len);
    // printk("%s\n",pbuf);
    // printk("%d\n",rspbuf.data[seq]);
    // return 0;
    // if(printk_ratelimit()){
    //     printk("send: %d\n",nlh->nlmsg_seq);
    // }
    nlmsg_unicast(nlsk, skb, USER_PORT);

    // if(printk_ratelimit()){
    //     printk("down signal:%d\n",rspbuf.sem[seq].count);
    //     printk("before data:%d\n",rspbuf.data[seq]);
    // }
    // return 1;

    if (down_timeout(&rspbuf.sem[seq], 3 * HZ)) {
        if (__ratelimit(&rs)) {
            printk(KERN_NOTICE "[fvault] fvault terminated!\n");
        }
        return 0;
    }
    return rspbuf.data[seq];
}

int send_usrmsg(char *pbuf, uint16_t len)
{
    struct sk_buff *nl_skb;
    struct nlmsghdr *nlh;
    int ret;
    /* 创建sk_buff 空间 */
    nl_skb = nlmsg_new(len, GFP_ATOMIC);
    if(!nl_skb)
    {
        printk("netlink alloc failure\n");
        return -1;
    }
    /* 设置netlink消息头部 */
    nlh = nlmsg_put(nl_skb, 0, 0, NETLINK_TEST, len, 0);
    if(nlh == NULL)
    {
        printk("nlmsg_put failaure \n");
        nlmsg_free(nl_skb);
        return -1;
    }
    /* 拷贝数据发送 */
    memcpy(nlmsg_data(nlh), pbuf, len);
    ret = netlink_unicast(nlsk, nl_skb, USER_PORT,MSG_DONTWAIT);
    return ret;
}

static void netlink_rcv_msg(struct sk_buff *skb)
{
    struct nlmsghdr* nlh = (struct nlmsghdr*)skb->data;
    char *umsg = NLMSG_DATA(nlh);
    if(skb->len >= nlmsg_total_size(0))
    {
        if (strcmp(umsg,"daemon start")==0)
        {
            daemon_flag=1;
        }
        else
        {
            rspbuf.data[nlh->nlmsg_seq] = (int)(umsg[0])-48;
            // if(printk_ratelimit()){
            //     printk("rev:%d\n",nlh->nlmsg_seq);
            //     printk("after data:%d\n",rspbuf.data[nlh->nlmsg_seq]);
            //     printk("up signal1:%d\n",rspbuf.sem[nlh->nlmsg_seq].count);
            // }
            up(&rspbuf.sem[nlh->nlmsg_seq]);
            // if(printk_ratelimit()){
            //     printk("up signal2:%d\n",rspbuf.sem[nlh->nlmsg_seq].count);
            // }
        }
    }
}

struct netlink_kernel_cfg cfg = {
        .input  = netlink_rcv_msg, /* set recv callback */
};

int test_netlink_init(void)
{
    int i;
    /* create netlink socket */
    nlsk = (struct sock *)netlink_kernel_create(&init_net, NETLINK_TEST, &cfg);
    if(nlsk == NULL)
    {
        printk("netlink_kernel_create error !\n");
        return -1;
    }
    for (i = 0; i < 65536; ++i) {
        rspbuf.data[i] = 0;
        sema_init(&rspbuf.sem[i], 0);
    }
    ratelimit_set_flags(&rs, RATELIMIT_MSG_ON_RELEASE);
    printk("test_netlink_init\n");
    return 0;
}

void test_netlink_exit(void)
{
    if (nlsk){
        netlink_kernel_release(nlsk); /* release ..*/
        nlsk = NULL;
    }
    printk("test_netlink_exit!\n");
}


