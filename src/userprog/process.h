#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/*============================= PA3 ADDED CODE =============================*/
/* This new struct PCB is to clarify data members belonging to processes that
the kernel will use for process syncronization, control, etc. 

GROUP NOTE: This is a member of struct thread in thread.h included when
compiled from userprog (which should be the working project directory for
priject 3).

1) PintOS is a simple OS. All processes running on the system are single
threaded.

2) The kernel manages all processes through the use of kernel threads, the
reasoning for this structure being included in thread.h.

3) If a kernel thread is for kernel use it is alright that it will contain
a pcb, it simply will not be utalized by the kernel.

4) For our projects process mapping is 1 to 1 therefore tid == pid && 
pid == tid and should be set as such. */

typedef int pid_t; /* Definition of a pit_t */

struct process_control_block
{
	pid_t  pid;       /* Process ID num */
	int exit_status;  /* Process exit status migrated from PA2 */
	/* List of children associated with this process */
	struct list child_list;      /* TODO Determine location */
	struct list_elem child_elem; /* This should be here */

	/*Consider migration of struct semaphore process_wait_sema and strict 
	*thread parent to this struct.*/
};
/*=========================== END PA3 ADDED CODE ===========================*/

#endif /* userprog/process.h */
