#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <linux/types.h>
#include <linux/dirent.h>
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


struct linux_dirent {
    unsigned long  d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    char           d_name[];
};

typedef void (* sys_call_ptr_t)(void);
typedef asmlinkage ssize_t (*old_syscall_t)(struct pt_regs* regs);
sys_call_ptr_t *get_sys_call_table(void);

old_syscall_t orig_openat=NULL;
old_syscall_t orig_read=NULL;
old_syscall_t orig_write=NULL;
old_syscall_t orig_execve=NULL;
old_syscall_t orig_getdents=NULL;

unsigned int level=0;
pte_t *pte=NULL;
sys_call_ptr_t * sys_call_table = NULL;

struct semaphore sem;
int check_result;
int daemon_flag=0;

int test_netlink_init(void);
void test_netlink_exit(void);
int send_usrmsg(char *pbuf, uint16_t len);
int check_privilege(char *pbuf, uint16_t len);

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
    uid_t uid;
    unsigned long ino;
    if (daemon_flag)
    {
        ino = get_ino_from_name(regs->di, (char*)regs->si);
        uid = current_uid().val;
        sprintf(check_msg,"%u %lu",uid,ino);
        if (uid==0 || ino==0 || check_privilege(check_msg,sizeof(check_msg)))
            ret = orig_openat(regs);
    }
    else
        ret = orig_openat(regs);
    return ret;
}

asmlinkage long hacked_read(struct pt_regs *regs)
{
    long ret=-1;
    char buffer[PATH_MAX];
    char check_msg[30];
    uid_t uid;
    unsigned long ino;
    if (daemon_flag)
    {
        ino = get_ino_from_fd(regs->di);
        uid = current_uid().val;
        sprintf(check_msg,"%u %lu",uid,ino);
        if (uid==0 || ino==0 || check_privilege(check_msg,sizeof(check_msg)))
            ret = orig_read(regs);
    }
    else
        ret = orig_read(regs);
    return ret;
}

asmlinkage long hacked_write(struct pt_regs *regs)
{
    long ret=-1;
    char buffer[PATH_MAX];
    char check_msg[30];
    uid_t uid;
    unsigned long ino;
    if (daemon_flag)
    {
        ino = get_ino_from_fd(regs->di);
        uid = current_uid().val;
        sprintf(check_msg,"%u %lu",uid,ino);
        if (uid==0 || ino==0 || check_privilege(check_msg,sizeof(check_msg)))
            ret = orig_write(regs);
    }
    else
        ret = orig_write(regs);
    return ret;
}

asmlinkage long hacked_execve(struct pt_regs* regs) 
{
    long ret=-1;
    char buffer[PATH_MAX];
    char check_msg[30];
    uid_t uid;
    unsigned long ino;
    if (daemon_flag)
    {
        ino = get_ino_from_name(AT_FDCWD, (char*)regs->di);// 文件名位于rdi
        uid = current_uid().val;
        sprintf(check_msg,"%u %lu",uid,ino);
        if (uid==0 || ino==0 || check_privilege(check_msg,sizeof(check_msg)))
            ret = orig_execve(regs);
    }
    else
        ret = orig_execve(regs);
    return ret;
}

asmlinkage long hacked_getdents(struct pt_regs* regs) 
{
    char check_msg[30];
    uid_t uid;
    ssize_t ret = -1;
    long copylen = 0;
    // 创建指针用于处理目录项
    //intro to getdents: https://man7.org/linux/man-pages/man2/getdents.2.html
    struct linux_dirent *filtered_dirent;
    struct linux_dirent *orig_dirent;
    struct linux_dirent *td1;
    struct linux_dirent *td2;

    uid = current_uid().val;
    // 使用原系统调用
    ret = orig_getdents(regs);

    // 处理空文件
    if (ret == 0) return ret;

    // 申请内核的内存空间
    filtered_dirent = (struct linux_dirent*)kmalloc(ret, GFP_KERNEL);
    td1 = filtered_dirent;
    orig_dirent = (struct linux_dirent*)kmalloc(ret, GFP_KERNEL);
    td2 = orig_dirent;
    // 将目录项复制到内核空间
    copy_from_user(orig_dirent, (void *)regs->si, ret);

    // 迭代检查目录项
    while (ret > 0) {
        ret -= td2->d_reclen;
        sprintf(check_msg,"%u %lu",uid,td2->d_ino);
        if (uid==0 || td2->d_ino==0 || check_privilege(check_msg,sizeof(check_msg))) {
            // 目录项通过检查
            memmove(td1, (char *)td2, td2->d_reclen);
            td1 = (struct linux_dirent*)((char *)td1 + td2->d_reclen);
            copylen += td2->d_reclen;
        }
        td2 = (struct linux_dirent*)((char *)td2 + td2->d_reclen);
    }
    // 目录项复制回到用户空间
    copy_to_user((void *)regs->si, filtered_dirent, copylen);
    // 释放内核态内存空间
    kfree(orig_dirent);
    kfree(filtered_dirent);

    return copylen;
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
    printk("Info:  orginal write:%lx\n",(long)orig_write);
    orig_execve = (old_syscall_t)sys_call_table[__NR_execve];
    printk("Info:  orginal execve:%lx\n",(long)orig_execve);
    orig_getdents = (old_syscall_t)sys_call_table[__NR_getdents];
    printk("Info:  orginal getdents:%lx\n",(long)orig_getdents);
    

    pte = lookup_address((unsigned long) sys_call_table, &level);
    // Change PTE to allow writing
    set_pte_atomic(pte, pte_mkwrite(*pte));
    printk("Info: Disable write-protection of page with sys_call_table\n");

    sys_call_table[__NR_openat] = (sys_call_ptr_t) hacked_openat;
    sys_call_table[__NR_read] = (sys_call_ptr_t) hacked_read;
    sys_call_table[__NR_write] = (sys_call_ptr_t) hacked_write;
    sys_call_table[__NR_execve] = (sys_call_ptr_t)hacked_execve;
    sys_call_table[__NR_getdents] = (sys_call_ptr_t)hacked_getdents;

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
    sys_call_table[__NR_execve] = (sys_call_ptr_t)orig_execve;
    sys_call_table[__NR_getdents] = (sys_call_ptr_t)orig_getdents;
    set_pte_atomic(pte, pte_clear_flags(*pte, _PAGE_RW));
    test_netlink_exit();

}

module_init(audit_init);
module_exit(audit_exit);
//
// Created by Administrator on 2020/10/29.
//

