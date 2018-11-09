#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void sys_exit(int status);
void close (int fd);
void close_file_from_process (int fd);

/*This lock is for protecting file system access*/
struct lock filesys;


#define CLOSE_ALL_FILES -1

#endif /* userprog/syscall.h */
