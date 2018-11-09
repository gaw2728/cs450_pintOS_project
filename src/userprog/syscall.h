#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void sys_exit(int status);
void close (int fd);
void close_file_from_process (int fd);

/*This lock is for protecting file system access*/
struct lock filesys;



/*This semaphore, initialized to 1, controls when writes are allowed.*/
struct semaphore write_allowed;
     
/*This lock is used to count the number of readers and to determine which of the readers will up or down write_allowed.*/
struct lock mutex;
     
/*An unsigned integer to keep track of the number of simultaneous readers in the system.*/
int reader_count;



#define CLOSE_ALL_FILES -1

#endif /* userprog/syscall.h */
