#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>	// for create_proc_info_entry() 
#include <linux/sched.h>	// for 'struct task_struct'
#include <linux/seq_file.h>	// for sequence files
#include <linux/mm.h>		// for 'struct mm_struct'
#include <linux/slab.h>       // for kzalloc, kfree  
#include <linux/uaccess.h>
char modname[] = "logadd2phyadd";
typedef struct data {
	unsigned long addr;
	int p;
}mydata;
static mydata indata;
static char tmp[1024];
static void get_pgtable_macro(struct seq_file *m)
{

   seq_printf( m,"PAGE_OFFSET = 0x%lx\n", PAGE_OFFSET);
   seq_printf( m,"PGDIR_SHIFT = %d\n", PGDIR_SHIFT);
   //输出进程P4D_SHIFT,PUD_SHIFT,PMD_SHIFT,PAGE_SHIFT?
	
   seq_printf( m,"PTRS_PER_PGD = %d\n", PTRS_PER_PGD);
   //输出进程PTRS_PER_P4D,PTRS_PER_PUD,PTRS_PER_PMD,PTRS_PER_PTE?

   seq_printf( m,"PAGE_MASK = 0x%lx\n", PAGE_MASK);


}

static unsigned long vaddr2paddr(struct seq_file *m, unsigned long vaddr,int pid)
{
    pte_t *pte_tmp = NULL;  
    pmd_t *pmd_tmp = NULL;  
    pud_t *pud_tmp = NULL;  
    p4d_t *p4d_tmp = NULL; 
    pgd_t *pgd_tmp = NULL;  
    struct task_struct *pcb_tmp = NULL;
    unsigned long paddr = 0;
   /*
    if(!(pcb_tmp = find_task_by_pid(pid))) {
         printk(KERN_INFO"Can't find the task %d .\n",pid);
         return 0;
    }
   */
    printk(KERN_INFO"in vaddr2paddr try to find the task %d .\n",pid);	
    pcb_tmp =pid_task(find_get_pid(pid),PIDTYPE_PID); 
    if(!pcb_tmp) {
         printk(KERN_INFO"Can't find the task %d .\n",pid);
         return 0;
    }
	
  pgd_tmp = pgd_offset(pcb_tmp->mm, vaddr);
  if (pgd_none(*pgd_tmp)) {
      printk("not mapped in pgd\n");
      return -1;
  }
  seq_printf( m,"pgd_tmp = 0x%p\n",pgd_tmp);
  seq_printf( m,"pgd_val(*pgd_tmp) = 0x%lx\n",pgd_val(*pgd_tmp));

  p4d_tmp = p4d_offset(pgd_tmp, vaddr);  
  if (p4d_none(*p4d_tmp)) {
      printk("not mapped in pud\n");
      return -1;
  }
  seq_printf( m,"p4d_tmp = 0x%p\n",p4d_tmp);
  seq_printf( m,"p4d_val(*pud_tmp) = 0x%lx\n",p4d_val(*p4d_tmp));	

  pud_tmp = pud_offset(p4d_tmp, vaddr);  
  if (pud_none(*pud_tmp)) {
      printk("not mapped in pud\n");
      return -1;
  }
  seq_printf( m,"pud_tmp = 0x%p\n",pud_tmp);
  seq_printf( m,"pud_val(*pud_tmp) = 0x%lx\n",pud_val(*pud_tmp));	

  pmd_tmp = pmd_offset(pud_tmp, vaddr);
  if (pmd_none(*pmd_tmp)) {
      printk("not mapped in pmd\n");
      return -1;
  }
  seq_printf( m,"pmd_tmp = 0x%p\n",pmd_tmp);
  seq_printf( m,"pmd_val(*pmd_tmp) = 0x%lx\n",pmd_val(*pmd_tmp));	

  //pte = pte_offset_kernel(pmd, vaddr);
  pte_tmp = pte_offset_kernel(pmd_tmp, vaddr);    
  if (pte_none(*pte_tmp)) {
      printk("not mapped in pte\n");
      return -1;
  }
  seq_printf( m,"pte_tmp = 0x%p\n",pte_tmp);
  seq_printf( m,"pte_val(*pte_tmp) = 0x%lx\n",pte_val(*pte_tmp));

    //ÎïÀíµØÖ·µÈÓÚ £šÖ¡µØÖ· | Æ«ÒÆÁ¿£©
  paddr = (pte_val(*pte_tmp) & PAGE_MASK) | (vaddr & ~PAGE_MASK); 
  seq_printf( m, "frame_addr = %lx\n", pte_val(*pte_tmp) & PAGE_MASK);
  seq_printf( m, "frame_offset = %lx\n", vaddr & ~PAGE_MASK);
  seq_printf( m, "the input logic address is = 0x%08lX\n", vaddr);
  seq_printf( m, "the corresponding physical address is= 0x%08lX\n",paddr);
  return paddr;
}

static int my_seq_show(struct seq_file *m, void *p)
{
        //unsigned int	_cr0,_cr3, _cr4;
	get_pgtable_macro(m);
	printk("the get_pgtable_macro has been output\n");
        if (indata.p !=0 && indata.addr !=0)
		vaddr2paddr(m, indata.addr,indata.p);
        printk("the vaddr2paddr has been done\n");		
	seq_printf( m, "\n" );
	
	return	0;

}

static void * my_seq_start(struct seq_file *m, loff_t *pos)
{
  if (0 == *pos)  
  {  
     ++*pos;  
     return (void *)1; 
   }  

        return NULL;
}
static void * my_seq_next(struct seq_file *m, void *p, loff_t *pos)
{
        // do nothing
        return NULL;
}
static void my_seq_stop(struct seq_file *m, void *p)
{
				//// do nothing        
}

static struct seq_operations my_seq_fops = {
        .start  = my_seq_start,
        .next   = my_seq_next,
        .stop   = my_seq_stop,
        .show   = my_seq_show
};

static int my_open(struct inode *inode, struct file *file)  
{  
    return seq_open(file, &my_seq_fops);  
}   

//ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
// file_operations -> write  
//static ssize_t my_write(struct file *file, const char __user *buffer, size_t count) 
static ssize_t my_write(struct file *file, const char __user *buffer, size_t count, loff_t *p) 
{  
    
    //char *tmp = kzalloc((count+1), GFP_KERNEL);  
    //if (!tmp)  
      //  return -ENOMEM;  

    //copy_to|from_user(to,from,cnt)  
    if (copy_from_user(tmp, buffer, count)) {  
        //kfree(tmp);  
        return -EFAULT;  
    } 
    indata = *(mydata*)tmp;     	
    printk("the useraddr is %lu\n",indata.addr);
    printk("the pid is %d\n",indata.p);	  

    //kfree(tmp); 

    return count;  
} 
static const struct file_operations my_proc =   
{  
      .owner      = THIS_MODULE,  
      .open       = my_open,
      .write      = my_write, 	
      .read       = seq_read,      
      .llseek     = seq_lseek,
      .release    = seq_release  	    
};
 
static int __init my_init( void )
{
	struct proc_dir_entry* my_proc_entry;
	printk( "<1>\nInstalling \'%s\' module\n", modname );
	my_proc_entry = proc_create(modname, 0x0666, NULL, &my_proc);
	if (NULL == my_proc_entry)
	{
	    return -ENOMEM;
	}
		return	0;  //SUCCESS
}

static void __exit my_exit(void )
{
	remove_proc_entry( modname, NULL );
	printk( "<1>Removing \'%s\' module\n", modname );	
}

module_init( my_init );
module_exit( my_exit );
MODULE_LICENSE("GPL"); 
