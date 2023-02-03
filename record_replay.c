/*
 *  linux/kernel/sys.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

//copying headers from fork
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/unistd.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/completion.h>
#include <linux/personality.h>
#include <linux/mempolicy.h>
#include <linux/sem.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/iocontext.h>
#include <linux/key.h>
#include <linux/binfmts.h>
#include <linux/mman.h>
#include <linux/mmu_notifier.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/vmacache.h>
#include <linux/nsproxy.h>
#include <linux/capability.h>
#include <linux/cpu.h>
#include <linux/cgroup.h>
#include <linux/security.h>
#include <linux/hugetlb.h>
#include <linux/seccomp.h>
#include <linux/swap.h>
#include <linux/syscalls.h>
#include <linux/jiffies.h>
#include <linux/futex.h>
#include <linux/compat.h>
#include <linux/kthread.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/rcupdate.h>
#include <linux/ptrace.h>
#include <linux/mount.h>
#include <linux/audit.h>
#include <linux/memcontrol.h>
#include <linux/ftrace.h>
#include <linux/proc_fs.h>
#include <linux/profile.h>
#include <linux/rmap.h>
#include <linux/ksm.h>
#include <linux/acct.h>
#include <linux/tsacct_kern.h>
#include <linux/cn_proc.h>
#include <linux/freezer.h>
#include <linux/delayacct.h>
#include <linux/taskstats_kern.h>
#include <linux/random.h>
#include <linux/tty.h>
#include <linux/blkdev.h>
#include <linux/fs_struct.h>
#include <linux/magic.h>
#include <linux/perf_event.h>
#include <linux/posix-timers.h>
#include <linux/user-return-notifier.h>
#include <linux/oom.h>
#include <linux/khugepaged.h>
#include <linux/signalfd.h>
#include <linux/uprobes.h>
#include <linux/aio.h>
#include <linux/compiler.h>
#include <linux/sysctl.h>

#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/uaccess.h>
#include <asm/mmu_context.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#include <trace/events/sched.h>


#include <trace/events/task.h>


static LIST_HEAD(records);
//static LIST_HEAD(sys_records);

SYSCALL_DEFINE1(start_record, int, record_id)
{
	if(record_id<0) return -EINVAL; //repace with error code INVALID ARG
	if(current->rec_id>0) return -EAGAIN;

	printk(KERN_INFO  "Hello from start_record \n");
	current->rec_id = record_id;

	return record_id;
}
SYSCALL_DEFINE2(stop_record, struct syscall_record __user *, record_buf, size_t, count)
{
	// struct syscall_record *sys;
	// struct record_dict *rec;
	struct record_dict *rec;
	struct record_dict *store;
	struct syscall_record *sys;
	// struct list_head *temp;
	int ret;
	int c=0;
	long n;
	bool replace =false;
	printk(KERN_INFO  "Initialize stop_record \n");
	if(record_buf==NULL) return -EINVAL;
	if(count<current->total_syscalls) return -EOVERFLOW;
	store = kmalloc(sizeof(struct record_dict *),GFP_KERNEL);
	// temp = kmalloc(sizeof(struct list_head *),GFP_KERNEL);
	//INIT_LIST_HEAD(temp);

	list_for_each_entry(sys,&current->record_list,sys_list){		//current->record_list check
		if(sys==NULL){
			printk(KERN_WARNING  "sys is null: %d \n",c);
			return -14; //EFAULT
		} 
        if(sys->syscall_nr<0) {
        	printk(KERN_WARNING  "syscall_nr<0: %d \n",sys->syscall_nr);
          	return -14; //EFAULT
      	}
          //printk(KERN_INFO "System call # %d returns: %ld \n", sys->syscall_nr,sys->rv);
          n = sizeof(struct syscall_record);	
          ret = copy_to_user (record_buf,sys,n);
          printk(KERN_INFO "System call # %d returns: %ld. Copied to %p with return value: %d \n", sys->syscall_nr,sys->rv,record_buf,ret);
          //kfree(sys);
          record_buf++;
          if(ret!=0) return -14; //EFAULT
          c++;
    }


    
	store->rec_id=current->rec_id;
	store->total_syscalls=current->total_syscalls;
	// INIT_LIST_HEAD(&store->sys_rec);
	list_replace_init(&current->record_list,&store->sys_rec);
	
	//adding record_id to the record dict list
	list_for_each_entry(rec, &records, list) {
		printk(KERN_INFO  "going through records id: %d \n", current->rec_id);
		if(rec->rec_id==current->rec_id){
			replace=true;
			list_del(&rec->list);
			break;
		} 
	}
	printk(KERN_INFO "Done Iterating, now inserting new record at record_id: %d in records \n",current->rec_id);
	list_add(&store->list,&records);
	// if(replace){
	// 	printk(KERN_WARNING "Freeing up previous record existing at record_id: %d \n",rec->rec_id);
	// 	list_del(&rec->sys_rec);
	// 	kfree(rec);
	// } 
	printk(KERN_INFO "total syscall: %d added successfully \n",current->total_syscalls);
	//reset the current variables
	current->rec_id = 0;
	current->total_syscalls=0;

	printk(KERN_INFO "Store Check: Total %d systemcalls for record id: %d \n",store->total_syscalls, store->rec_id);
	//for debug of store
	// list_for_each_entry(sys,&store->sys_rec,sys_list){		//store check
	// 	if(sys==NULL){
	// 		printk(KERN_WARNING  "sys is null: %d \n",c);
	// 		return -14; //EFAULT
	// 	} 
 //        if(sys->syscall_nr<0) {
 //        	printk(KERN_WARNING  "syscall_nr<0: %d \n",sys->syscall_nr);
 //          	return -14; //EFAULT
 //      	}	
 //        printk(KERN_EMERG "System call # %d returns: %ld \n", sys->syscall_nr,sys->rv);
 //    }

	return c;

}



SYSCALL_DEFINE3(start_replay, int, record_id, struct syscall_record __user *, record_buf, size_t, count)
{
	struct syscall_record *sys;
	struct record_dict *rec;
	struct syscall_record store[count];
	bool rec_found=false;
	long fd=0,fdc=0;
	int flags=0;
	off_t offset=0;
	size_t bufsize=0;
	umode_t mode=0;
	const char __user * filename=NULL;
	const char __user *buf=NULL;
	char __user *ogbuf=NULL;
	char __user * usrbuf=NULL;
	int c=0,j=0;
	//int recovery[count];
	long ret, cret,n,rret;
	printk(KERN_INFO  "Hello from start_replay \n");
	ret=0;
	if(current->rec_id>0) return -EBUSY;
	if(record_buf==NULL) return -EINVAL;
	
	//find the record
	list_for_each_entry(rec, &records, list) {
		printk(KERN_INFO  "going through records id: %d \n", rec->rec_id);
		if(rec->rec_id==record_id){
			rec_found=true;
			break;
		} 
	}

	if(!rec_found) return -EBADF;
	if(count<rec->total_syscalls) return -EOVERFLOW;

	printk(KERN_WARNING "Replaying %d systemcalls in rec of record id: %d \n",rec->total_syscalls, rec->rec_id);
	
	list_for_each_entry(sys,&rec->sys_rec,sys_list){		
        printk(KERN_WARNING "Originally System call # %d returns: %ld \n", sys->syscall_nr,sys->rv);
        switch(sys->syscall_nr){
        	
        	case 2 :
        	printk(KERN_INFO "Re-Executing Open \n");
        	ret=sys_open((const char __user *)sys->args[0],(int) sys->args[1],(umode_t) sys->args[2]);
        	fd=ret;
        	flags=sys->args[1];
        	filename=(const char __user *)sys->args[0];
        	mode = sys->args[2];
        	break;

        	case 1 :
        	printk(KERN_INFO "Re-Executing Write \n");
        	ret=sys_write((unsigned int)sys->args[0],(const char __user *)sys->args[1],(size_t)sys->args[2]);
        	buf=(const char __user *)sys->args[1];
        	break;
        	
        	case 8 :
        	printk(KERN_INFO "Re-Executing lseek \n");
        	ret=sys_lseek((unsigned int)sys->args[0],(off_t)sys->args[1],(unsigned int)sys->args[2]);
        	offset=sys->args[1];
        	break;

        	case 0 :
        	printk(KERN_INFO "Re-Executing Read \n");
        	ret=sys_read((unsigned int)sys->args[0],(char __user *)sys->args[1],(size_t)sys->args[2]);
        	usrbuf=(char __user *)sys->args[1];
        	bufsize= sys->args[2];
        	break;
        	
        	case 3 :
        	printk(KERN_INFO "Re-Executing Close \n");
        	ret=sys_close((unsigned int)sys->args[0]);
        	break;

        	case 85 :
        	printk(KERN_INFO "Re-Executing Creat \n");
        	ret=sys_creat((const char __user *)sys->args[0],(umode_t)sys->args[1]);
        	filename=(const char __user *)sys->args[0];
        	fdc=ret;
        	break;

        	case 77 :
        	printk(KERN_INFO "Re-Executing ftruncate \n");
        	ret=sys_ftruncate((unsigned int)sys->args[0],(unsigned long)sys->args[1]);
        	break;
        	default:
        		printk(KERN_INFO "No matching case found \n");
        		ret=-EINVAL;
        		break;
        }
        
        sys->rv =ret;
        if(ret<0) break;
        //recovery[c]=[sys->syscall_nr];
        store[c]=*sys;
        printk(KERN_WARNING "Replayed System call # %d returns: %ld \n", sys->syscall_nr,ret);
        c++;
        n = sizeof(struct syscall_record);	
          cret = copy_to_user (record_buf,sys,n);
          printk(KERN_WARNING "Replayed System call # %d returns: %ld. Copied to %p with return value: %ld \n", sys->syscall_nr,sys->rv,record_buf,cret);
          record_buf++;
        if(c>=rec->total_syscalls) break;
    }
    //recovery
    if(ret<0){
    	for(j=c-1;j>=0;j--){
    		switch(store[j].syscall_nr){
	        	case 2 :
	        	printk(KERN_INFO "Recovering Open \n");
	        	rret=sys_close(fd);
	        	//sys_open((const char __user *)store[j].args[0],(int) store[j].args[1],(umode_t) store[j].args[2]);
	        	break;

	        	case 1 :
	        	printk(KERN_INFO "Recovering Write \n");
	        	//rret=sys_write((unsigned int)store[j].args[0],(const char __user *)store[j].args[1],(size_t)store[j].args[2]);
	        	sys_read(fd, ogbuf, bufsize);
	        	sys_write(fd,ogbuf,bufsize);
	        	break;
	        	
	        	case 8 :
	        	printk(KERN_INFO "Recovering lseek \n");
	        	//rret=sys_lseek((unsigned int)store[j].args[0],(off_t)store[j].args[1],(unsigned int)store[j].args[2]);
	        	sys_lseek(fd, SEEK_SET, 0);
	        	break;

	        	case 0 :
	        	printk(KERN_INFO "Recovering Read \n");
	        	//rret=sys_read((unsigned int)store[j].args[0],(char __user *)store[j].args[1],(size_t)store[j].args[2]);
	        	rret=copy_to_user(&usrbuf,&buf,sizeof(char)*strlen(buf)+1);
	        	break;
	        	
	        	case 3 :
	        	printk(KERN_INFO "Recovering Close \n");
	        	//rret=sys_close((unsigned int)store[j].args[0]);
	        	sys_open(filename,flags,mode);
	        	break;

	        	case 85 :
	        	printk(KERN_INFO "Recovering Creat \n");
	        	//rret=sys_creat((const char __user *)store[j].args[0],(umode_t)store[j].args[1]);
	        	rret=sys_close(fdc);
	        	sys_unlink(filename);
	        	break;

	        	case 77 :
	        	printk(KERN_INFO "Recovering ftruncate \n");
	        	//rret=sys_ftruncate((unsigned int)store[j].args[0],(unsigned long)store[j].args[1]);
	        	sys_ftruncate(fd,offset);
	        	break;
	        	default:
	        		printk(KERN_INFO "No matching case found \n");
	        		rret=-EINVAL;
	        		break;
        	}
    	}
    }

	return c;
}

