#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/utsname.h>
#include <asm/pgtable.h>
#include <linux/kallsyms.h>
#include <asm/uaccess.h>
#include <asm/ptrace.h>
#include <linux/limits.h>
#include <linux/sched.h>
#include <linux/ctype.h>

typedef void (* demo_sys_call_ptr_t)(void);

demo_sys_call_ptr_t * get_sys_call_table(void)
{
    demo_sys_call_ptr_t * _sys_call_table=NULL;

    _sys_call_table=(demo_sys_call_ptr_t *)kallsyms_lookup_name("sys_call_table");

    //Print sys_call_table address
    printk("Info: _sys_call_table is at %lx\n", (long) _sys_call_table);

    return _sys_call_table;
}
