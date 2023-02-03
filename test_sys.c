#define SYS_start_record 326

#define SYS_stop_record  327

#define SYS_start_replay 328



#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define list_entry(ptr, type, member) \
  container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
  list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member) \
  list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_for_each_entry(pos, head, member)        \
  for (pos = list_first_entry(head, typeof(*pos), member);  \
       &pos->member != (head);          \
       pos = list_next_entry(pos, member))
int ret =10;
struct list_head {
  struct list_head *next, *prev;
};

struct record_dict{
  int rec_id;
  int total_syscalls;
  struct list_head list;
  struct list_head sys_rec;
};

struct syscall_record{
  
  int syscall_nr;           /* syscall no : storing count of syscalls*/
  unsigned long args[6];    /* arguments given by user space */
  long rv;                  /* return value */
  struct list_head sys_list;

};
int start_record(int record_id)

{

    asm volatile (

        "syscall"                  // Use syscall instruction

        : "=a" (ret)               // Return value in RAX

        : "0"(SYS_start_record),   // Syscall numer

          "D"(record_id)           // 1st parameter in RDI (D)

        : "rcx", "r11", "memory"); // clobber list (RCX and R11)

    return ret;

}


int stop_record(struct syscall_record *recordbuf, size_t count) {

    asm volatile (

        "syscall"                  // Use syscall instruction

        : "=a" (ret)               // Return value in RAX

        : "0"(SYS_stop_record),   // Syscall numer

          "D"(recordbuf),           // 1st parameter in RDI (D)
          "S"(count)

        : "rcx", "r11", "memory"); // clobber list (RCX and R11)

    return ret;

}


int start_replay(int record_id, struct syscall_record *recordbuf, size_t count) {

     asm volatile (

        "syscall"                  // Use syscall instruction

        : "=a" (ret)               // Return value in RAX

        : "0"(SYS_start_replay),   // Syscall numer

          "D"(record_id),           // 1st parameter in RDI (D)
          "S"(recordbuf),
          "d"(count)

        : "rcx", "r11", "memory"); // clobber list (RCX and R11)

    return ret;

}

int main(void)
{
    // const char hello[] = "Hello world!\n";
    // printf("%s",hello);
    // printf("start_record %d \n",start_record(1));
    // printf("stop_record %d \n",stop_record(NULL,5));
    // printf("start_replay %d \n",start_replay(1,NULL,5));
  int rep;
    ret = start_record(2);
    struct syscall *sys;
    if (ret < 0) {
        printf("start_record has failed (rv = %d)\n", ret);
    }

    int fd = open("./test", O_CREAT | O_RDWR, S_IRUSR);
    write(fd, "Hello World", 11);
    lseek(fd, SEEK_SET, 0);
    char buf[16];
    read(fd, &buf, 16);
    close(fd);
    struct syscall_record record[16];    //for testing since syscall is more of what structure is required
    struct syscall_record replay[16];
    ret = stop_record(&record[0], 16);

    if (ret < 0) {
        printf("stop_record has failed (rv = %d)\n", ret);

    } else {
        //printf("system calls are recorded with total sys calls: %ld \n", record[0].sys_nr);
        for(int i=0;i<ret;i++){
          printf("system call# %d  returns: %ld at record addr: %p \n", record[i].syscall_nr,record[i].rv,&record[i]);
          for(int j=0;j<record[i].args[5];j++){
            if(record[i].args[j]<0) break;
            printf("arg# %d : %lu \n",j,record[i].args[j]);
          }
        }

        /* The following system calls should be recorded and returned:
              open
              write
              lseek
              read
              close

        */
        
    }   
    rep=start_replay(5,&replay[0],16);
    printf("replay returns: %d \n",rep);
    for(int i=0;i<rep;i++){
          printf("Replayed system call# %d  returns: %ld at record addr: %p \n", replay[i].syscall_nr,replay[i].rv,&replay[i]);
          for(int j=0;j<replay[i].args[5];j++){
            if(replay[i].args[j]<0) break;
            printf("arg# %d : %lu \n",j,replay[i].args[j]);
          }
        }
    //printf("rec_id: %d, total syscalls: %d \n",record[0].rec_id,record[0].syscall_nr);
    return ret;
    
}