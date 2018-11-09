#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"
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
typedef int pid_t;
#define PID_ERROR -1
#define PID_INIT  -2

pid_t process_execute (const char *file_name);
int process_wait (pid_t);
void process_exit (void);
void process_activate (void);

// holds general information about a file
// TODO: CONSIDER REFACTORING FIELD NAMES
struct open_file {
  struct file *file;     // filesys file pointer
  int fd;                // file descriptor
  struct list_elem file_elem; // represents this file in the file_list
};

struct process_control_block
{
	/*=============== MEMBERS FOR PROCESS CREATION AND SYNC ===============*/
	pid_t pid;                    /* Process ID num */
	int exit_status;              /* Process exit status migrated from PA2 */
	const char* cmd_line;         /* added for new process_execute */
	struct thread *parent;        /* pointer to this thread's parent thread*/
	struct list_elem child_elem;  /* child List element */
	bool parent_waiting;          /* to see if a child is being waited on */
	bool child_exit;              /* to notify that a child has exited */
	bool orphan_flag;             /* to tell if a child process is orphaned*/

	/*semaphore for the initialization of a new process.*/
	struct semaphore process_init_sema;
	/*Semaphore utalized from last project, salvaged*/
	struct semaphore process_wait_sema;
	/*=============== MEMBERS FOR PROCESS CREATION AND SYNC ===============*/
};
/*=========================== END PA3 ADDED CODE ===========================*/

#endif /* userprog/process.h */
