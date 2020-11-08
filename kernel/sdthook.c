#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include <asm/ptrace.h>
#include <linux/string.h>
#include <linux/semaphore.h>
/*
** module macros
*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("infosec-sjtu");
MODULE_DESCRIPTION("hook sys_call_table");


typedef void (* sys_call_ptr_t)(void);
typedef asmlinkage ssize_t (*old_syscall_t)(struct pt_regs* regs);
sys_call_ptr_t *get_sys_call_table(void);

old_syscall_t orig_openat=NULL;
old_syscall_t orig_read=NULL;
old_syscall_t orig_write=NULL;

unsigned int level=0;
pte_t *pte=NULL;
sys_call_ptr_t * sys_call_table = NULL;

struct semaphore sem;
int check_result;

int test_netlink_init(void);
void test_netlink_exit(void);
int send_usrmsg(char *pbuf, uint16_t len);

static unsigned long get_ino_from_fd(unsigned int fd) {
    // 通过文件描述符获得文件控制块
    struct fd f = fdget(fd);
    umode_t mode = 0;
    unsigned long ino = 0;
    if (!IS_ERR(f.file)) {
        // 检查文件类型，排除设备文件
        mode = f.file->f_inode->i_mode;
        if (!S_ISCHR(mode) && !S_ISBLK(mode)) {
            ino = f.file->f_inode->i_ino;
        }
        fdput(f);
    }
    return ino;
}

static unsigned long get_ino_from_name(int dfd, const char* filename) {
    struct kstat stat;
    umode_t mode = 0;
    unsigned long ino = 0;
    // 通过文件名获得文件属性
    int error = vfs_statx(dfd, filename, AT_NO_AUTOMOUNT, &stat, STATX_BASIC_STATS);

    if (!error) {
        // 检查文件类型，排除设备文件
        mode = stat.mode;
        if (!S_ISCHR(mode) && !S_ISBLK(mode)) {
            ino = stat.ino;
        }
    }
    return ino;
}

asmlinkage long hacked_openat(struct pt_regs *regs)
{
    long ret=-1;
    char buffer[PATH_MAX];
    char check_msg[30];
	char log[PATH_MAX + 25];
    char tmp[6];
    long nbytes;
    uid_t uid;
    unsigned long ino;
    nbytes=strncpy_from_user(buffer,(char*)regs->bx,PATH_MAX);
    strncpy(tmp,buffer,5);
    tmp[5]='\0';
    if (strcmp(tmp,"/home")==0)
    {
        ino = get_ino_from_name(regs->di, (char*)regs->si);
        uid = current_uid().val;
        printk("%s %u %lu",buffer,uid,ino);
        sprintf(check_msg,"%u %lu",uid,ino);
        send_usrmsg(check_msg,sizeof(check_msg));
        down(&sem);
        if (check_result)
            ret = orig_openat(regs);
		else {   //操作失败时向用户层发送审计信息，包括文件路径、uid、访问方式
			strcpy(log, buffer);
			strcat(log, ":open:");
			strcat(log, uid);
			send_usrmsg(log, sizeof(log));
		}
        printk("%d\n",check_result);
        check_result=0;
        // protect file can only be put into /home directory
    }
    else
        ret = orig_openat(regs);
    return ret;
}

asmlinkage ssize_t hacked_read(struct pt_regs* regs) {
    long ret=-1;
    char buffer[PATH_MAX];
    char check_msg[30];
	char log[PATH_MAX + 25];
    char tmp[6];
    long nbytes;
    uid_t uid;
    unsigned long ino;
    nbytes=strncpy_from_user(buffer,(char*)regs->bx,PATH_MAX);
    strncpy(tmp,buffer,5);
    tmp[5]='\0';
    if (strcmp(tmp,"/home")==0)
    {
        ino = get_ino_from_name(regs->di, (char*)regs->si);
        uid = current_uid().val;
        printk("%s %u %lu",buffer,uid,ino);
        sprintf(check_msg,"%u %lu",uid,ino);
        send_usrmsg(check_msg,sizeof(check_msg));
        down(&sem);
        if (check_result)
            ret = orig_read(regs);
		else {   //操作失败时向用户层发送审计信息，包括文件路径、uid、访问方式
			strcpy(log, buffer);
			strcat(log, ":read:");
			strcat(log, uid);
			send_usrmsg(log, sizeof(log));
		}
        printk("%d\n",check_result);
        check_result=0;
        // protect file can only be put into /home directory
    }
    else
        ret = orig_read(regs);
    return ret;
}

asmlinkage ssize_t hacked_write(struct pt_regs* regs) {
    long ret=-1;
    char buffer[PATH_MAX];
    char check_msg[30];
	char log[PATH_MAX + 25];
    char tmp[6];
    long nbytes;
    uid_t uid;
    unsigned long ino;
    nbytes=strncpy_from_user(buffer,(char*)regs->bx,PATH_MAX);
    strncpy(tmp,buffer,5);
    tmp[5]='\0';
    if (strcmp(tmp,"/home")==0)
    {
        ino = get_ino_from_name(regs->di, (char*)regs->si);
        uid = current_uid().val;
        printk("%s %u %lu",buffer,uid,ino);
        sprintf(check_msg,"%u %lu",uid,ino);
        send_usrmsg(check_msg,sizeof(check_msg));
        down(&sem);
        if (check_result)
            ret = orig_write(regs);
		else {   //操作失败时向用户层发送审计信息，包括文件路径、uid、访问方式
			strcpy(log, buffer);
			strcat(log, ":write:");
			strcat(log, uid);
			send_usrmsg(log, sizeof(log));
		}
        printk("%d\n",check_result);
        check_result=0;
        // protect file can only be put into /home directory
    }
    else
        ret = orig_write(regs);
    return ret;
}

static int __init audit_init(void)
{
    sys_call_table = get_sys_call_table();
    printk("Info: sys_call_table found at %lx\n",(unsigned long)sys_call_table) ;

    //Hook Sys Call Openat
    orig_openat = (old_syscall_t)sys_call_table[__NR_openat];
    printk("Info:  orginal openat:%lx\n",(long)orig_openat);
    orig_read = (old_syscall_t)sys_call_table[__NR_read];
    printk("Info:  orginal read:%lx\n",(long)orig_read);
    orig_write = (old_syscall_t)sys_call_table[__NR_write];
    printk("Info:  orginal read:%lx\n",(long)orig_read);


    pte = lookup_address((unsigned long) sys_call_table, &level);
    // Change PTE to allow writing
    set_pte_atomic(pte, pte_mkwrite(*pte));
    printk("Info: Disable write-protection of page with sys_call_table\n");

    sys_call_table[__NR_openat] = (sys_call_ptr_t) hacked_openat;
    sys_call_table[__NR_read] = (sys_call_ptr_t) hacked_read;
    sys_call_table[__NR_write] = (sys_call_ptr_t) hacked_write;

    set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
    printk("Info: sys_call_table hooked!\n");

    test_netlink_init();
    sema_init(&sem, 0);

    return 0;
}


static void __exit audit_exit(void)
{
    pte = lookup_address((unsigned long) sys_call_table, &level);
    set_pte_atomic(pte, pte_mkwrite(*pte));
    sys_call_table[__NR_openat] = (sys_call_ptr_t)orig_openat;
    sys_call_table[__NR_read] = (sys_call_ptr_t)orig_read;
    sys_call_table[__NR_write] = (sys_call_ptr_t)orig_write;
    set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
    test_netlink_exit();

}

module_init(audit_init);
module_exit(audit_exit);
//
// Created by Administrator on 2020/10/29.
//
