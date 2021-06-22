# Linux进程虚拟地址空间分析



## 目录

1. [原理分析](#1.原理分析)
   1. [进程虚拟地址空间布局](#1.1进程虚拟地址空间布局)
      1. [栈](#1.1.1栈)
      2. [内存映射区](#1.1.2内存映射区)
      3. [堆](#1.1.3堆)
      4. [BSS段](#1.1.4BSS段)
      5. [Data段](#1.1.5Data段)
      6. [Text段](#1.1.6Text段)
   2. [管理地址空间的数据结构](#1.2管理地址空间的数据结构)
      1. [vm_area_struct](#1.2.1vm_area_struct)
      2. [mm_struct](#1.2.2mm_struct)
   3. [查看进程地址空间布局](#1.3查看进程地址空间布局)
2. [实验部分](#2.实验部分)
   1. [变量存储位置](#2.1变量存储位置)
      1. [常规变量](#2.1.1常规变量)
      2. [字符串常量](#2.1.2字符串常量)
      3. [malloc动态分配](#2.1.3malloc动态分配)
   2. [brk和sbrk](#2.2brk和sbrk)
   3. [内存映射区](#2.3内存映射区)
      1. [mmap代码剖析](#2.3.1mmap代码剖析)
      2. [文件共享](#2.3.2文件共享)
      3. [静态和动态链接库](#2.3.3静态和动态链接库)
3. [参考文献](#3.参考文献)





















## 1.原理分析

### 1.1进程虚拟地址空间布局

在Linux中，每个进程都拥有独立的虚拟地址空间，其又分为内核空间和用户空间。用户空间布局按照**由高地址到低地址**的次序，依次为**环境变量、命令行参数、栈、存储映射区、堆、BSS段、Data段、Text段**。以32位地址为例，进程虚拟地址空间布局如下图所示（图片来源[1]）。

<img src="images\1.jpg" style="zoom:80%;" />



#### 1.1.1栈

栈又称堆栈，**由编译器自动分配释放**，行为类似数据结构中的栈(**先进后出**)。堆栈主要有三个用途[1]：

- 为函数内部声明的**非静态局部变量**提供存储空间。
- **记录函数调用过程相关的维护性信息**，称为**栈帧**(Stack Frame)或**过程活动记录**(Procedure Activation Record)。由于栈的后进先出特点，所以栈特别方便用来**保存/恢复调用现场**。
- **临时存储区**，用于暂存长算术表达式部分计算结果或alloca()函数分配的栈内内存。



#### 1.1.2内存映射区

内存映射是一种方便高效的文件I/O方式， 因而被用于**装载动态共享库**。用户也可创建**匿名内存映射**，该映射没有对应的文件, 可用于存放程序数据。在 Linux中，若通过malloc()请求一大块内存，C运行库将创建一个匿名内存映射，而不使用堆内存。”大块” 意味着比阈值 MMAP_THRESHOLD还大，缺省为128KB，可通过mallopt()调整。



#### 1.1.3堆

堆用于存放**进程运行时动态分配的内存段**，可动态扩张或缩减。堆中内容是匿名的，不能按名字直接访问，只能通过指针间接访问。当进程调用malloc等函数分配内存时，新分配的内存动态添加到堆上(扩张)；当调用free等函数释放内存时，被释放的内存从堆中剔除(缩减) 。

分配的堆内存是经过字节对齐的空间，以适合原子操作。堆管理器通过链表管理每个申请的内存，由于堆申请和释放是无序的，最终会产生内存碎片。**堆内存一般由应用程序分配释放，回收的内存可供重新使用。若程序员不释放，程序结束时操作系统可能会自动回收。**

堆的末端由break指针标识，当堆管理器需要更多内存时，可通过系统调用brk()和sbrk()来移动break指针以扩张堆，一般由系统自动调用。



#### 1.1.4BSS段

通常是指用来存放程序中**未初始化或初始化为0的全局变量和静态局部变量**的一块内存区域。**BSS段仅为未初始化的静态分配变量预留位置，在目标文件中并不占据空间**，这样可减少目标文件体积。但程序运行时需为变量分配内存空间，故目标文件必须记录所有未初始化的静态分配变量大小总和(通过start_bss和end_bss地址写入机器代码)。当加载器(loader)加载程序时，将为BSS段分配的内存初始化为0。



#### 1.1.5Data段

Data段通常用于存放程序中**已初始化且初值不为0的全局变量和静态局部变量**。



#### 1.1.6Text段

Text段也称正文段或文本段，通常是指用来**存放程序执行代码**的一块内存区域。这部分区域的大小在程序运行前就已经确定，并且内存区域通常属于只读, 某些架构也允许代码段为可写，即允许修改程序。在代码段中，也有可能包含一些**只读的常数变量**，例如字符串常量等。

**代码段最容易受优化措施影响。**



> Linux通过对栈、内存映射段、堆的起始地址加上**随机偏移量**来打乱布局，**以免恶意程序通过计算访问栈、库函数等地址**。如果需要，当然也可以让程序的栈和映射区域从一个固定位置开始，只需要设置全局变量randomize_va_space值为0即可（默认值为 1）。



### 1.2管理地址空间的数据结构

一个进程的虚拟地址空间主要由两个数据结来描述。一个是最高层次的：mm_struct，一个是较高层次的：vm_area_struct。最高层次的**mm_struct结构描述了一个进程的整个虚拟地址空间**。较高层次的结构**vm_area_truct描述了虚拟地址空间的一个区间**（简称虚拟区）。它们都定义在include\linux\mm_types.h 文件中。两种数据结构管理进程地址空间如下图所示（图片来源[2]）：

<img src="images\2.gif" style="zoom:80%;" />



#### 1.2.1vm_area_struct

Linux通过类型为vm_area_struct的对象实现一个memory region。每个memory region描述符表示一个线性地址区间。

vm_start字段指向线性区的第一个线性地址，而vm_end 字段指向线性区之后的第一个线性地址。因此vm_end - vm_start表示memory region的长度。vm_mm字段指向拥有这个区间的进程的mm_struct内存描述符。

**进程所拥有的memory region从来不重叠，并且内核尽力把新分配的memory region与紧邻的现有memory region进行合并。两个相邻memory region的访问权限如果相匹配，就能把它们合并在一起。**

为了存放进程的线性区，**Linux既使用了链表，也使用了红黑树**。这两种数据结构包含指向同一线性区描述符的指针，当插入或删除一个 线性区描述符时，内核通过红黑树搜索前后元素，并用搜索结果快速更新链表而不用扫描链表。

**链表**：链表的头由内存描述符的mmap字段所指向。任何线性区对象 都在vm_next字段存放指向链表下一个元素的指针，在vm_prev字段存 放指向链表上一个元素的指针。

**红黑树**：红黑树的首部由内存描述符中的mm_rb字段所指向。任何线性区对象都在类型为rb_node的vm_rb字段中存放颜色以及指向左孩子 和右孩子的指针。

vm_area_struct定义如下：

```c
/*
 * This struct defines a memory VMM memory area. There is one of these
 * per VM-area/task.  A VM area is any part of the process virtual memory
 * space that has a special rule for the page-fault handlers (ie a shared
 * library, the executable area etc).
 */
struct vm_area_struct {
	/* The first cache line has the info for VMA tree walking. */

	unsigned long vm_start;		/* Our start address within vm_mm. */
	unsigned long vm_end;		/* The first byte after our end address
					   within vm_mm. */

	/* linked list of VM areas per task, sorted by address */
	struct vm_area_struct *vm_next, *vm_prev;

	struct rb_node vm_rb;

	/*
	 * Largest free memory gap in bytes to the left of this VMA.
	 * Either between this VMA and vma->vm_prev, or between one of the
	 * VMAs below us in the VMA rbtree and its ->vm_prev. This helps
	 * get_unmapped_area find a free area of the right size.
	 */
	unsigned long rb_subtree_gap;

	/* Second cache line starts here. */

	struct mm_struct *vm_mm;	/* The address space we belong to. */
	pgprot_t vm_page_prot;		/* Access permissions of this VMA. */
	unsigned long vm_flags;		/* Flags, see mm.h. */

	/*
	 * For areas with an address space and backing store,
	 * linkage into the address_space->i_mmap interval tree.
	 */
	struct {
		struct rb_node rb;
		unsigned long rb_subtree_last;
	} shared;

	/*
	 * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
	 * list, after a COW of one of the file pages.	A MAP_SHARED vma
	 * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
	 * or brk vma (with NULL file) can only be in an anon_vma list.
	 */
	struct list_head anon_vma_chain; /* Serialized by mmap_sem &
					  * page_table_lock */
	struct anon_vma *anon_vma;	/* Serialized by page_table_lock */

	/* Function pointers to deal with this struct. */
	const struct vm_operations_struct *vm_ops;

	/* Information about our backing store: */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
					   units */
	struct file * vm_file;		/* File we map to (can be NULL). */
	void * vm_private_data;		/* was vm_pte (shared mem) */

#ifdef CONFIG_SWAP
	atomic_long_t swap_readahead_info;
#endif
#ifndef CONFIG_MMU
	struct vm_region *vm_region;	/* NOMMU mapping region */
#endif
#ifdef CONFIG_NUMA
	struct mempolicy *vm_policy;	/* NUMA policy for the VMA */
#endif
	struct vm_userfaultfd_ctx vm_userfaultfd_ctx;
} __randomize_layout;

struct core_thread {
	struct task_struct *task;
	struct core_thread *next;
};
```



#### 1.2.2mm_struct

内核数据结构mm_struct中的成员变量start_code和end_code是进程代码段的起始和终止地址，start_data和 end_data是进程数据段的起始和终止地址，start_stack是进程堆栈段起始地址，start_brk是进程动态内存分配起始地址（堆的起始地址），还有一个 brk（堆的当前最后地址），就是动态内存分配当前的终止地址。

进程描述符中的**mm字段指向进程所拥有的内存描述符**，而**active_mm字段指向进程运行时所使用的内存描述符**。对于**普通进程，这两个字段存放相同的指针**。 但是，对于**内核线程**，由于内核线程不拥有任何内存描述符，因此，它们的**mm字段总是NULL** 。当**内核线程运行时，它的active_mm字段被初始化为前一个运行进程的active_mm值**[3]。

mm_struct定义如下：

```c
struct mm_struct {
	struct {
		struct vm_area_struct *mmap;		/* list of VMAs */
		struct rb_root mm_rb;
		u64 vmacache_seqnum;                   /* per-thread vmacache */
#ifdef CONFIG_MMU
		unsigned long (*get_unmapped_area) (struct file *filp,
				unsigned long addr, unsigned long len,
				unsigned long pgoff, unsigned long flags);
#endif
		unsigned long mmap_base;	/* base of mmap area */
		unsigned long mmap_legacy_base;	/* base of mmap area in bottom-up allocations */
#ifdef CONFIG_HAVE_ARCH_COMPAT_MMAP_BASES
		/* Base adresses for compatible mmap() */
		unsigned long mmap_compat_base;
		unsigned long mmap_compat_legacy_base;
#endif
		unsigned long task_size;	/* size of task vm space */
		unsigned long highest_vm_end;	/* highest vma end address */
		pgd_t * pgd;

		/**
		 * @mm_users: The number of users including userspace.
		 *
		 * Use mmget()/mmget_not_zero()/mmput() to modify. When this
		 * drops to 0 (i.e. when the task exits and there are no other
		 * temporary reference holders), we also release a reference on
		 * @mm_count (which may then free the &struct mm_struct if
		 * @mm_count also drops to 0).
		 */
		atomic_t mm_users;

		/**
		 * @mm_count: The number of references to &struct mm_struct
		 * (@mm_users count as 1).
		 *
		 * Use mmgrab()/mmdrop() to modify. When this drops to 0, the
		 * &struct mm_struct is freed.
		 */
		atomic_t mm_count;

#ifdef CONFIG_MMU
		atomic_long_t pgtables_bytes;	/* PTE page table pages */
#endif
		int map_count;			/* number of VMAs */

		spinlock_t page_table_lock; /* Protects page tables and some
					     * counters
					     */
		struct rw_semaphore mmap_sem;

		struct list_head mmlist; /* List of maybe swapped mm's.	These
					  * are globally strung together off
					  * init_mm.mmlist, and are protected
					  * by mmlist_lock
					  */


		unsigned long hiwater_rss; /* High-watermark of RSS usage */
		unsigned long hiwater_vm;  /* High-water virtual memory usage */

		unsigned long total_vm;	   /* Total pages mapped */
		unsigned long locked_vm;   /* Pages that have PG_mlocked set */
		atomic64_t    pinned_vm;   /* Refcount permanently increased */
		unsigned long data_vm;	   /* VM_WRITE & ~VM_SHARED & ~VM_STACK */
		unsigned long exec_vm;	   /* VM_EXEC & ~VM_WRITE & ~VM_STACK */
		unsigned long stack_vm;	   /* VM_STACK */
		unsigned long def_flags;

		spinlock_t arg_lock; /* protect the below fields */
		unsigned long start_code, end_code, start_data, end_data;
		unsigned long start_brk, brk, start_stack;
		unsigned long arg_start, arg_end, env_start, env_end;

		unsigned long saved_auxv[AT_VECTOR_SIZE]; /* for /proc/PID/auxv */

		/*
		 * Special counters, in some configurations protected by the
		 * page_table_lock, in other configurations by being atomic.
		 */
		struct mm_rss_stat rss_stat;

		struct linux_binfmt *binfmt;

		/* Architecture-specific MM context */
		mm_context_t context;

		unsigned long flags; /* Must use atomic bitops to access */

		struct core_state *core_state; /* coredumping support */
#ifdef CONFIG_MEMBARRIER
		atomic_t membarrier_state;
#endif
#ifdef CONFIG_AIO
		spinlock_t			ioctx_lock;
		struct kioctx_table __rcu	*ioctx_table;
#endif
#ifdef CONFIG_MEMCG
		/*
		 * "owner" points to a task that is regarded as the canonical
		 * user/owner of this mm. All of the following must be true in
		 * order for it to be changed:
		 *
		 * current == mm->owner
		 * current->mm != mm
		 * new_owner->mm == mm
		 * new_owner->alloc_lock is held
		 */
		struct task_struct __rcu *owner;
#endif
		struct user_namespace *user_ns;

		/* store ref to file /proc/<pid>/exe symlink points to */
		struct file __rcu *exe_file;
#ifdef CONFIG_MMU_NOTIFIER
		struct mmu_notifier_mm *mmu_notifier_mm;
#endif
#if defined(CONFIG_TRANSPARENT_HUGEPAGE) && !USE_SPLIT_PMD_PTLOCKS
		pgtable_t pmd_huge_pte; /* protected by page_table_lock */
#endif
#ifdef CONFIG_NUMA_BALANCING
		/*
		 * numa_next_scan is the next time that the PTEs will be marked
		 * pte_numa. NUMA hinting faults will gather statistics and
		 * migrate pages to new nodes if necessary.
		 */
		unsigned long numa_next_scan;

		/* Restart point for scanning and setting pte_numa */
		unsigned long numa_scan_offset;

		/* numa_scan_seq prevents two threads setting pte_numa */
		int numa_scan_seq;
#endif
		/*
		 * An operation with batched TLB flushing is going on. Anything
		 * that can move process memory needs to flush the TLB when
		 * moving a PROT_NONE or PROT_NUMA mapped page.
		 */
		atomic_t tlb_flush_pending;
#ifdef CONFIG_ARCH_WANT_BATCHED_UNMAP_TLB_FLUSH
		/* See flush_tlb_batched_pending() */
		bool tlb_flush_batched;
#endif
		struct uprobes_state uprobes_state;
#ifdef CONFIG_HUGETLB_PAGE
		atomic_long_t hugetlb_usage;
#endif
		struct work_struct async_put_work;

#ifdef CONFIG_HMM_MIRROR
		/* HMM needs to track a few things per mm */
		struct hmm *hmm;
#endif
	} __randomize_layout;

	/*
	 * The mm_cpumask needs to be at the end of mm_struct, because it
	 * is dynamically sized based on nr_cpu_ids.
	 */
	unsigned long cpu_bitmap[];
};
```



### 1.3查看进程地址空间布局

在Linux中，运行一个可执行程序，在程序执行完毕前，都可以通过**虚拟文件系统**来查看进程的**虚拟地址空间布局的详细信息**。

例如，以test程序为例，代码如下：

```c
//test.c
int main(){
    while(1);
    return 0;
}
```

编译链接生成可执行文件`gcc test.c -o test`，在当前目录下执行该文件`./test`，则运行该程序的进程将一直存在。通过`ps aux | grep test`获得test进程的pid，然后通过`cat /proc/pid/maps`查看进程地址空间映射。该过程如下图所示：

<img src="images\3.jpg" style="zoom: 80%;" />

映射信息的详细说明如下图说明（图片来源[3]）：

<img src="images\4.jpg" style="zoom: 80%;" />



## 2.实验部分

### 2.1变量存储位置

#### 2.1.1常规变量

以C语言为例，变量根据**定义位置**的不同可分为**局部/全局**，**共2种**；根据**初始化的情况**可分为**未初始化/初始化为0/初始化为非0**，**共3种**；根据**关键字修饰情况**可分为**无关键字修饰/static/const/static const**，**共4种**。因此，变量的种类可达**2\*3\*4=24种**之多。为了明确每一种变量具体所处的位置，设计如下的测试程序lkpmem.c：

```c
//lkpmem.c
#include <stdio.h>
#include <stdlib.h>

//未初始化：u 初始化为零：o　初始化为非零：i
//静态：sta　常量：con　局部：loc　全局：glo

//全局
int u_glo1;
int o_glo1 = 0;
int i_glo1 =1;
//静态－全局
static int u_sta_glo1;
static int o_sta_glo1 = 0;
static int i_sta_glo1 =1;
//常量－全局
const int u_con_glo1;
const int o_con_glo1 = 0;
const int i_con_glo1 =1;
//静态－常量－全局
static const int u_sta_con_glo1;
static const int o_sta_con_glo1 = 0;
static const int i_sta_con_glo1 =1;

int main()
{
//局部
int u_loc1;
int o_loc1 = 0;
int i_loc1 =1;
//静态－局部
static int u_sta_loc1;
static int o_sta_loc1 = 0;
static int i_sta_loc1 =1;
//常量－局部
const int u_con_loc1;
const int o_con_loc1 = 0;
const int i_con_loc1 =1;
//静态－常量－局部
static const int u_sta_con_loc1;
static const int o_sta_con_loc1 = 0;
static const int i_sta_con_loc1 =1;


printf("u_glo1 \tvalue:%d\t location: 0x%lx\n",u_glo1, &u_glo1);
printf("o_glo1 \tvalue:%d\t location: 0x%lx\n",o_glo1, &o_glo1);
printf("i_glo1 \tvalue:%d\t location: 0x%lx\n",i_glo1, &i_glo1);
printf("\n");
printf("u_sta_glo1 \tvalue:%d\t location: 0x%lx\n",u_sta_glo1, &u_sta_glo1);
printf("o_sta_glo1 \tvalue:%d\t location: 0x%lx\n",o_sta_glo1, &o_sta_glo1);
printf("i_sta_glo1 \tvalue:%d\t location: 0x%lx\n",i_sta_glo1, &i_sta_glo1);
printf("\n");
printf("u_con_glo1 \tvalue:%d\t location: 0x%lx\n",u_con_glo1, &u_con_glo1);
printf("o_con_glo1 \tvalue:%d\t location: 0x%lx\n",o_con_glo1, &o_con_glo1);
printf("i_con_glo1 \tvalue:%d\t location: 0x%lx\n",i_con_glo1, &i_con_glo1);
printf("\n");
printf("u_sta_con_glo1 \tvalue:%d\t location: 0x%lx\n",u_sta_con_glo1, &u_sta_con_glo1);
printf("o_sta_con_glo1 \tvalue:%d\t location: 0x%lx\n",o_sta_con_glo1, &o_sta_con_glo1);
printf("i_sta_con_glo1 \tvalue:%d\t location: 0x%lx\n",i_sta_con_glo1, &i_sta_con_glo1);
printf("\n");
printf("u_loc1 \tvalue:%d\t location: 0x%lx\n",u_loc1, &u_loc1);
printf("o_loc1 \tvalue:%d\t location: 0x%lx\n",o_loc1, &o_loc1);
printf("i_loc1 \tvalue:%d\t location: 0x%lx\n",i_loc1, &i_loc1);
printf("\n");
printf("u_sta_loc1 \tvalue:%d\t location: 0x%lx\n",u_sta_loc1, &u_sta_loc1);
printf("o_sta_loc1 \tvalue:%d\t location: 0x%lx\n",o_sta_loc1, &o_sta_loc1);
printf("i_sta_loc1 \tvalue:%d\t location: 0x%lx\n",i_sta_loc1, &i_sta_loc1);
printf("\n");
printf("u_con_loc1 \tvalue:%d\t location: 0x%lx\n",u_con_loc1, &u_con_loc1);
printf("o_con_loc1 \tvalue:%d\t location: 0x%lx\n",o_con_loc1, &o_con_loc1);
printf("i_con_loc1 \tvalue:%d\t location: 0x%lx\n",i_con_loc1, &i_con_loc1);
printf("\n"); 
printf("u_sta_con_loc1 \tvalue:%d\t location: 0x%lx\n",u_sta_con_loc1, &u_sta_con_loc1);
printf("o_sta_con_loc1 \tvalue:%d\t location: 0x%lx\n",o_sta_con_loc1, &o_sta_con_loc1);
printf("i_sta_con_loc1 \tvalue:%d\t location: 0x%lx\n",i_sta_con_loc1, &i_sta_con_loc1);

while(1);
return(0);
}
```

编译链接生成可执行程序lkpmem，运行结果如下图所示：

<img src="images\5.jpg" style="zoom: 80%;" />

查看进程的地址空间映射，结果如下图所示：

<img src="images\6.jpg" style="zoom: 80%;" />

结合上述结果，统计出24种变量所处的区域，统计结果如下图所示，需要注意的是，**系统将.bss和.data两个段合并为了一个段，并且用4B区分两个部分**，如下图的**0x55eb2769f01c~0x55eb2769f01f这四个字节未使用**。

<img src="images\7.jpg" style="zoom: 80%;" />

> Linux采用的是**小端方式存储数据**。设计如下测试程序以验证：
>
> ```c
> //temp.c
> #include <stdio.h>
> int a = 0x0A0B0C0D;
> int main(){
> 
>     printf("a \tvalue:%d\t location: 0x%lx\n",a, &a);
>     printf("location: 0x%lx\tvalue:0x%lx\n",(char*)&a,*((char*)&a));
>     printf("location: 0x%lx\tvalue:0x%lx\n",(char*)&a+1,*((char*)&a+1));
>     printf("location: 0x%lx\tvalue:0x%lx\n",(char*)&a+2,*((char*)&a+2));
>     printf("location: 0x%lx\tvalue:0x%lx\n",(char*)&a+3,*((char*)&a+3));
>     return 0;
> }
> ```
>
> 运行结果如下图所示：
>
> <img src="images\8.jpg" style="zoom: 80%;" />



#### 2.1.2字符串常量

设计如下测试程序以确认字符串常量存储位置：

```c
//temp2.c
#include <stdio.h>
char *strglo = "ABC";
int main(){
    char *strloc = "abc";
    printf("\"ABC\"\t location: 0x%lx\n", strglo);
    printf("\"abc\"\t location: 0x%lx\n", strloc);
    while(1);
    return 0;
}
```

运行结果及进程地址空间映射信息如下图所示，可以看到**字符串常量均位于.text段**
<img src="images\9.jpg" style="zoom: 80%;" />



#### 2.1.3malloc动态分配

malloc()用于在进程运行时动态分配内存。**使用malloc()申请内存时，小于等于128k的将直接在堆中分配，若堆的空间不足，则使用brk()扩展堆的大小；大于128k的调用mmap()在内存映射区分配。**

编写如下测试程序验证：

```c
//temp3.c
#include <stdio.h>
int main(){
    void *small = (void *)malloc(128*1024);
    void *big = (void *)malloc(128*1024+1);
    printf("small\t location: 0x%lx\n", small);
    printf("big\t location: 0x%lx\n", big);
    while(1);
    return 0;
}
```

程序运行结果及对应进程地址空间映射如下图所示，可以看到使用malloc()申请**128K**大小的内存时，malloc()返回的内存空间首地址位于**堆**中；而申请**128K+1B**大小的内存时，返回的内存空间首地址位于**内存映射区**。

<img src="images\10.jpg" style="zoom: 80%;" />

> 需要注意的是，无论是brk()或mmap()，开辟的空间都只是虚拟地址空间，**并没有与内存物理地址建立真正的映射**。 只有当访问到该区域时，才会通过异常机制中的**缺页中断**，进行地址空间的映射。
>
> 那么为什么malloc要分以上两种情况呢？brk()分配的低地址内存需要在高地址内存释放后才能真正释 放，这就意味着堆中的低地址空间虽然释放了，但只是显示为空闲状态，该部分映射的内存物理空间并 没有还给操作系统，这就导致内存泄漏和碎片问题；但是由于这部分并没有还给操作系统，因此可重 用，并且访问该部分很可能不会再产生缺页中断，这会降低时间开销。另一方面，在文件映射部分中， mmap()申请的每一部分都会以结构体的形式使用链表或者树形结构链接，管理这部分也是不小的开 销。**结合brk()和mmap()两者的特点，小空间由brk()来申请，以减少碎片带来的影响；而大空间由 mmap()来申请，可以减少管理相关数据结构带来的开销。**可通过 mallopt(M_MMAP_THRESHOLD, ) 来修改这个临界值(默认128k)。



综合以上三部分，结果如下图所示：

<img src="images\11.png" style="zoom: 80%;" />



### 2.2brk和sbrk

由Linux-5.3/arch/x86/entry/syscalls/syscall_64.tbl文件可知，**brk()是第12号系统调用**。sbrk()也是常用的扩展堆空间的函数。二者的定义均位于Linux-5.3/tools/include/nolibc/nolibc.h文件中，代码如下：

```c
//Linux-5.3/tools/include/nolibc/nolibc.h
#define my_syscall1(num, arg1)                                                \
({                                                                            \
	long _ret;                                                            \
	register long _num asm("eax") = (num);                                \
	register long _arg1 asm("ebx") = (long)(arg1);                        \
									      \
	asm volatile (                                                        \
		"int $0x80\n"                                                 \
		: "=a" (_ret)                                                 \
		: "r"(_arg1),                                                 \
		  "0"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_ret;                                                                 \
})

static __attribute__((unused))
void *sys_brk(void *addr)
{
	return (void *)my_syscall1(__NR_brk, addr);
}

static __attribute__((unused))
int brk(void *addr)
{
	void *ret = sys_brk(addr);

	if (!ret) {
		SET_ERRNO(ENOMEM);
		return -1;
	}
	return 0;
}

static __attribute__((unused))
void *sbrk(intptr_t inc)
{
	void *ret;

	/* first call to find current end */
	if ((ret = sys_brk(0)) && (sys_brk(ret + inc) == ret + inc))
		return ret + inc;

	SET_ERRNO(ENOMEM);
	return (void *)-1;
}
```

由源代码可知，**brk通过传递的addr来重新设置program break，成功则返回0，否则返回-1。而sbrk用来增加堆，增加的大小通过参数inc决定，若成功，则返回增加大小前的堆的program break；若失败，则返回-1。**

需要特别注意的是，**进程第一次调用sbrk()，如果成功，则返回的是堆的起始地址。**另外，**堆的初始大小为132K**。

设计如下测试程序以验证：

```c
#include<stdio.h>
#include<unistd.h>

int main(){
    //第一次调用sbrk返回堆的起始地址
    printf("init start location\t0x%lx\n",sbrk(0));
    //sbrk(0)返回堆增长前的终止地址，因为增长为0，所以也是当前的终止地址
    printf("init end location\t0x%lx\n",sbrk(0));
    sbrk(4*1024);
    //查看sbrk增长4K后的地址
    printf("sbrk increased end location\t0x%lx\n",sbrk(0));
	//在当前终止地址上在增加4K，调用brk直接将program break设置为增长后的终止地址
    int ans = brk(sbrk(0)+4*1024);
    if(ans==0)
        printf("brk increased end location\t0x%lx\n",sbrk(0));
    
    while(1);
}
```

程序运行结果及对应进程地址空间映射如下图所示，进程默认堆的大小为0x557d3d8d0000-0x557d3d8af000，即132K。sbrk(4\*1024)增长后，堆终止地址为0x557d3d8d1000；brk(sbrk(0)+4*1024)后，堆终止地址为0x557d3d8d2000。

<img src="images\12.jpg" style="zoom: 80%;" />



### 2.3内存映射区

#### 2.3.1mmap代码剖析

mmap()是一种内存映射文件的方法，即**将一个文件或者其它对象映射到进程的地址空间**，实现文件磁 盘地址和进程虚拟地址空间中一段虚拟地址的一一对映关系。**实现这样的映射关系后，进程就可以采用指针的方式读写操作这一段内存，而系统会自动回写脏页面到对应的文件磁盘上，即完成了对文件的操作而不必再调用read,write等系统调用函数**。相反，内核空间对这段区域的修改也直接反映用户空间， 从而可以实现不同进程间的文件共享。

vm_area_struct结构中包含区域起始和终止地址以及其他相关信息，**mmap函数就是要创建一个新的 vm_area_struct结构，并将其与文件的物理磁盘地址相连**。

mmap的函数原型如下：

```c
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
```

**参数解释：**

- **addr**：如果不为NULL，内核会在此地址创建映射；否则，内核会选择一个合适的虚拟地址。大部 分情况不指定虚拟地址，意义不大，而是让内核选择返回一个地址给用户空间使用。 
- **length**：表示映射到进程地址空间的大小。 
- **prot**：内存区域的读/写/执行属性。
- **flags**：内存映射的属性，共享、私有、匿名、文件等。
- **fd**：表示这是一个文件映射，fd是打开文件的句柄。如果是文件映射，需要指定fd；匿名映射就指 定一个特殊的-1。 
- **offset**：在文件映射时，表示相对文件头的偏移量；返回的地址是偏移量对应的虚拟地址。

其中**flags可以有四种组合**： 

- **私有文件映射**：多个进程使用同样的物理页面进行初始化，但是各个进程对内存文件的修改不会共 享，也不会反映到物理文件中。比如对linux .so动态库文件就采用这种方式映射到各个进程虚拟地 址空间中。 
- **私有匿名映射**：mmap会创建一个新的映射，各个进程不共享，主要用于分配内存(malloc分配大 内存会调用mmap)。 
- **共享文件映射**：多个进程通过虚拟内存技术共享同样物理内存，对内存文件的修改会反应到实际物 理内存中，也是进程间通信的一种。 
- **共享匿名映射**：这种机制在进行fork时不会采用写时复制，父子进程完全共享同样的物理内存页， 也就是父子进程通信。

通过查询Linux-5.3/arch/x86/entry/syscalls/syscall_64.tbl，**mmap是9号系统调用，入口函数为sys_mmap**，代码如下：

```c
// Linux-5.3/arch/ia64/kernel/sys_ia64.c
asmlinkage unsigned long
sys_mmap (unsigned long addr, unsigned long len, int prot, int flags, int fd, long off)
{
	if (offset_in_page(off) != 0)
		return -EINVAL;

	addr = ksys_mmap_pgoff(addr, len, prot, flags, fd, off >> PAGE_SHIFT);
	if (!IS_ERR((void *) addr))
		force_successful_syscall_return();
	return addr;
}
```

**ksys_mmap_pgoff**代码如下：

```c
// Linux-5.3/mm/mmap.c
unsigned long ksys_mmap_pgoff(unsigned long addr, unsigned long len,
			      unsigned long prot, unsigned long flags,
			      unsigned long fd, unsigned long pgoff)
{
	struct file *file = NULL;
	unsigned long retval;

	if (!(flags & MAP_ANONYMOUS)) {//对非匿名文件映射的检查，必须能根据文件句柄找到struct file。
		audit_mmap_fd(fd, flags);
		file = fget(fd);
		if (!file)
			return -EBADF;
		if (is_file_hugepages(file))
			len = ALIGN(len, huge_page_size(hstate_file(file)));//根据file->f_op来判断是否是hugepage，然后进行hugepage页面对齐。
		retval = -EINVAL;
		if (unlikely(flags & MAP_HUGETLB && !is_file_hugepages(file)))
			goto out_fput;
	} else if (flags & MAP_HUGETLB) {
		struct user_struct *user = NULL;
		struct hstate *hs;

		hs = hstate_sizelog((flags >> MAP_HUGE_SHIFT) & MAP_HUGE_MASK);
		if (!hs)
			return -EINVAL;

		len = ALIGN(len, huge_page_size(hs));
		/*
		 * VM_NORESERVE is used because the reservations will be
		 * taken when vm_ops->mmap() is called
		 * A dummy user value is used because we are not locking
		 * memory so no accounting is necessary
		 */
		file = hugetlb_file_setup(HUGETLB_ANON_FILE, len,
				VM_NORESERVE,
				&user, HUGETLB_ANONHUGE_INODE,
				(flags >> MAP_HUGE_SHIFT) & MAP_HUGE_MASK);
		if (IS_ERR(file))
			return PTR_ERR(file);
	}

	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);

	retval = vm_mmap_pgoff(file, addr, len, prot, flags, pgoff);
out_fput:
	if (file)
		fput(file);
	return retval;
}
```

**vm_mmap_pgoff**代码如下：

```c
// Linux-5.3/mm/util.c
unsigned long vm_mmap_pgoff(struct file *file, unsigned long addr,
	unsigned long len, unsigned long prot,
	unsigned long flag, unsigned long pgoff)
{
	unsigned long ret;
	struct mm_struct *mm = current->mm;
	unsigned long populate;
	LIST_HEAD(uf);

	ret = security_mmap_file(file, prot, flag);
	if (!ret) {
		if (down_write_killable(&mm->mmap_sem))
			return -EINTR;
		ret = do_mmap_pgoff(file, addr, len, prot, flag, pgoff,
				    &populate, &uf);
		up_write(&mm->mmap_sem);
		userfaultfd_unmap_complete(mm, &uf);
		if (populate)
			mm_populate(ret, populate);
	}
	return ret;
}
```

**do_mmap_pgoff**代码如下：

```c
//  Linux-5.3/include/linux/mm.h
static inline unsigned long
do_mmap_pgoff(struct file *file, unsigned long addr,
	unsigned long len, unsigned long prot, unsigned long flags,
	unsigned long pgoff, unsigned long *populate,
	struct list_head *uf)
{
	return do_mmap(file, addr, len, prot, flags, 0, pgoff, populate, uf);
}
```

**do_mmap()是整个mmap()的具体操作函数，do_mmap()根据用户传入的参数做了一系列的检查，然后根据参数初始化vm_area_struct的标志 vm_flags，vma->vm_file = get_file(file)建立文件与vma的映射。具体步骤如下：**

1. **检查参数，并根据传入的映射类型设置vma的flags** 
2. **进程查找其虚拟地址空间，找到一块空闲的满足要求的虚拟地址空间**
3. **根据找到的虚拟地址空间初始化vma**
4. **设置vma->vm_file** 
5. **根据文件系统类型，将vma->vm_ops设为对应的file_operations**
6. **将vma插入mm的链表中**



**do_mmap**代码如下[4]：

```c
// Linux-5.3/mm/mmap.c
unsigned long do_mmap(struct file *file, unsigned long addr,
                        unsigned long len, unsigned long prot,
                        unsigned long flags, vm_flags_t vm_flags,
                        unsigned long pgoff, unsigned long *populate,
                        struct list_head *uf)
{
        struct mm_struct *mm = current->mm;	/* 获取该进程的memory descriptor*/
        int pkey = 0;
        *populate = 0;
	/*
	  函数对传入的参数进行一系列检查, 假如任一参数出错，都会返回一个errno
	 */
        if (!len)
                return -EINVAL;
 
        /*
         * Does the application expect PROT_READ to imply PROT_EXEC?
         *
         * (the exception is when the underlying filesystem is noexec
         *  mounted, in which case we dont add PROT_EXEC.)
         */
        if ((prot & PROT_READ) && (current->personality & READ_IMPLIES_EXEC))
                if (!(file && path_noexec(&file->f_path)))
                        prot |= PROT_EXEC;
    	/* force arch specific MAP_FIXED handling in get_unmapped_area */
        if (flags & MAP_FIXED_NOREPLACE)
                flags |= MAP_FIXED;
		
    	/* 假如没有设置MAP_FIXED标志，且addr小于mmap_min_addr, 因为可以修改addr, 所以就需要将addr设为mmap_min_addr的页对齐后的地址 */
        if (!(flags & MAP_FIXED))
                addr = round_hint_to_min(addr);
 
        /* Careful about overflows.. */
	/* 进行Page大小的对齐 */
        len = PAGE_ALIGN(len);
        if (!len)
                return -ENOMEM;
 
        /* offset overflow? */
        if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
                return -EOVERFLOW;
 
        /* Too many mappings? */
	/* 判断该进程的地址空间的虚拟区间数量是否超过了限制 */
        if (mm->map_count > sysctl_max_map_count)
                return -ENOMEM;
 
        /* Obtain the address to map to. we verify (or select) it and ensure
         * that it represents a valid section of the address space.
         */
    	/* get_unmapped_area从当前进程的用户空间获取一个未被映射区间的起始地址 */
        addr = get_unmapped_area(file, addr, len, pgoff, flags);
	/* 检查addr是否有效 */
    	if (offset_in_page(addr))
                return addr;
 
	/*  假如flags设置MAP_FIXED_NOREPLACE，需要对进程的地址空间进行addr的检查. 如果搜索发现存在重合的vma, 返回-EEXIST。
	    这是MAP_FIXED_NOREPLACE标志所要求的
	*/
        if (flags & MAP_FIXED_NOREPLACE) {
                struct vm_area_struct *vma = find_vma(mm, addr);
 
                if (vma && vma->vm_start < addr + len)
                        return -EEXIST;
        }
 
        if (prot == PROT_EXEC) {
                pkey = execute_only_pkey(mm);
                if (pkey < 0)
                        pkey = 0;
        }
 
        /* Do simple checking here so the lower-level routines won't have
         * to. we assume access permissions have been handled by the open
         * of the memory object, so we don't do any here.
         */
        vm_flags |= calc_vm_prot_bits(prot, pkey) | calc_vm_flag_bits(flags) |
                        mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;
    	/* 假如flags设置MAP_LOCKED，即类似于mlock()将申请的地址空间锁定在内存中, 检查是否可以进行lock*/
    	if (flags & MAP_LOCKED)
                if (!can_do_mlock())
                        return -EPERM;
 
        if (mlock_future_check(mm, vm_flags, len))
                return -EAGAIN;
    
    	if (file) { /* file指针不为nullptr, 即从文件到虚拟空间的映射 */
                struct inode *inode = file_inode(file); /* 获取文件的inode */
                unsigned long flags_mask;
 
                if (!file_mmap_ok(file, inode, pgoff, len))
                        return -EOVERFLOW;
 
                flags_mask = LEGACY_MAP_MASK | file->f_op->mmap_supported_flags;
 
                /*
                  ...
                  根据标志指定的map种类，把为文件设置的访问权考虑进去。
		  如果所请求的内存映射是共享可写的，就要检查要映射的文件是为写入而打开的，而不
		  是以追加模式打开的，还要检查文件上没有上强制锁。
		  对于任何种类的内存映射，都要检查文件是否为读操作而打开的。
		  ...
		*/
        } else {
                switch (flags & MAP_TYPE) {
                case MAP_SHARED:
                        if (vm_flags & (VM_GROWSDOWN|VM_GROWSUP))
                                return -EINVAL;
                        /*
                         * Ignore pgoff.
                         */
                        pgoff = 0;
                        vm_flags |= VM_SHARED | VM_MAYSHARE;
                        break;
                case MAP_PRIVATE:
                        /*
                         * Set pgoff according to addr for anon_vma.
                         */
                        pgoff = addr >> PAGE_SHIFT;
                        break;
                default:
                        return -EINVAL;
                }
        }
    	/*
         * Set 'VM_NORESERVE' if we should not account for the
         * memory use of this mapping.
         */
        if (flags & MAP_NORESERVE) {
                /* We honor MAP_NORESERVE if allowed to overcommit */
                if (sysctl_overcommit_memory != OVERCOMMIT_NEVER)
                        vm_flags |= VM_NORESERVE;
 
                /* hugetlb applies strict overcommit unless MAP_NORESERVE */
                if (file && is_file_hugepages(file))
                        vm_flags |= VM_NORESERVE;
        }
 
        addr = mmap_region(file, addr, len, vm_flags, pgoff, uf);
        if (!IS_ERR_VALUE(addr) &&
            ((vm_flags & VM_LOCKED) ||
             (flags & (MAP_POPULATE | MAP_NONBLOCK)) == MAP_POPULATE))
                *populate = len;
        return addr;
```



#### 2.3.2文件共享

**设计两个测试程序，均使用mmap()将同一文件映射至内存映射区。两个程序分别对相应内存映射区进行读或写，以验证文件共享**。测试程序如下：

```c
//openfile1.c
#include<stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(){
    int len = 32;
    char *filename = "./sharedfile";
    int fd = open(filename,O_RDWR );
    char *addr;
    addr = mmap(0, len, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
    printf("0x%lx\n",addr);

    char c = 'A';
    int i=0;
    while (1)
    {
        for(int j =0;j<len-1;j++){
             *(addr+j)=c;
        }
        i = (i+1)%26;
        c = 'A'+i;
    }

    return 0;
}

//openfile2.c
#include<stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(){
    int len = 32;
    char *filename = "./sharedfile";
    int fd = open(filename,O_RDWR );
    char *addr;
    addr = mmap(0, len, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

    while (1)
    {
        printf("%s",addr);
    }

    return 0;
}
```

其中sharedfile的内容如下，包含句末的\n共32个字符。

```
I love Linux's kernel analysis.
```

现在先运行读程序openfile2，运行结果如下图所示：

<img src="images\13.jpg" style="zoom: 80%;" />

**保持openfile2处于运行状态**，运行写程序openfile1，结果如下图所示：

<img src="images\14.jpg" style="zoom: 80%;" />

终止openfile1，此时openfile2的结果如下图所示：

<img src="images\15.jpg" style="zoom: 80%;" />

openfile1进程运行期间的地址空间如下图所示：

<img src="images\16.jpg" style="zoom: 80%;" />

openfile2进程运行期间的地址空间如下图所示：

<img src="images\17.jpg" style="zoom: 80%;" />

可以看到，**/home/wuhao/sharedfile/sharedfile文本文件均出现在两个进程的内存映射区**。通过测试可以说明，**进程之间可以通过mmap共享文件，以实现进程间通信。**



#### 2.3.3静态和动态链接库

Linux 下的**静态链接库**是以`.a`结尾的二进制文件，**它作为程序的一个模块，在链接期间被组合到程序中**。和静态链接库相对的是**动态链接库**（`.so`文件），它**在程序运行阶段被加载进内存**。

设计如下链接程序和测试程序：

```c
//linkfile.c
#include <stdio.h>

void mylibfoo()
{
  printf("here is link function.\n");
}

//libtest.c
#include <stdio.h>

int main()
{
  mylibfoo();
  while(1);
  return 0;
}
```

将链接程序创建为静态链接库liblinkfile.a，并静态链接到测试程序

```shell
#1.编译linkfile.c生成目标文件linkfile.o
gcc -c linkfile.c -o linkfile.o
#2.将linkfile.o创建为静态链接库liblinkfile.a
ar rcs liblinkfile.a linkfile.o
#3.编译libtest并链接静态链接库liblinkfile.a，生成atest
gcc libtest.c liblinkfile.a -o atest
```

将链接程序创建为动态链接库liblinkfile.so，并动态链接到测试程序

```shell
#1.编译链接linkfile.c为动态链接库liblinkfile.so
gcc -fPIC -shared linkfile.c -o liblinkfile.so
#2.拷贝动态库liblinkfile.so到默认动态库路径
sudo cp liblinkfile.so /usr/lib/
#3.编译libtest并链接动态链接库liblinkfile.so，生成sotest
gcc libtest.c liblinkfile.so -o sotest
```

静态链接生成的程序atest运行时的进程地址空间映射如下图所示：

<img src="images\18.jpg" style="zoom: 80%;" />

动态链接生成的程序sotest运行时的进程地址空间映射如下图所示：

<img src="images\19.jpg" style="zoom: 80%;" />

atest文件大小如下图所示，8368字节。

<img src="images\20.jpg" style="zoom: 80%;" />

sotest文件大小如下图所示，8296字节。

<img src="images\21.jpg" style="zoom: 80%;" />

可以看到**动态链接**生成的程序**sotest运行时**，会将动态链接库**liblinkfile.so映射到进程的内存映射区**，而**静态链接**生成的程序**atest**，**在生成时**就已经将需要的内容从静态链接库**liblinkfile.a中拷贝到atest中**了，这一点从两者的文件大小也可看出：**atest要大于sotest。**



## 3.参考文献

[1]linux进程虚拟地址空间https://www.cnblogs.com/beixiaobei/p/10507462.html

[2]linux内存管理之sys_brk实现分析【一】https://blog.csdn.net/BeyondHaven/article/details/6636561?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase

[3]北京大学软件与微电子学院《Linux内核分析和驱动编程》课件

[4]mmap源码分析https://blog.csdn.net/lggbxf/article/details/94012088