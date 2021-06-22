#include <linux/kernel.h> /* printk() */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h> /* task_struct */

int route_struct_init(void)
{
	struct task_struct *task = current;
	int i;
	printk(KERN_NOTICE "entering module");
	printk(KERN_NOTICE "0: This task is called %s, his pid is %d\n", task->comm, task->pid);
	for (i = 1; task->pid != 0; ++i)
	{
		//让task指向他的父进程
		//打印这个进程的comm、pid信息
	}
	return 0;
}

void route_struct_exit(void)
{
	printk(KERN_NOTICE "exiting module");
}

module_init(route_struct_init);
module_exit(route_struct_exit);
MODULE_LICENSE("Dual BSD/GPL");
