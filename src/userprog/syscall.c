#include "devices/input.h"
#include "devices/shutdown.h"
#include "list.h"
#include "process.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include <console.h>
#include <stdio.h>
#include <syscall-nr.h>

static void syscall_handler(struct intr_frame *);
/******************** PA2 ADDED CODE ********************/
void get_arguments(struct intr_frame *f, int *args, int num_of_args);
void check_address(const void *ptr_to_check);
void exit(int status);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
/******************** END PA2 ADDED CODE ********************/

void syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Get stack arguments and make system calls */
static void syscall_handler(struct intr_frame *f) {
  /******************** PA3 MODIFIED CODE ********************/
  /* Holds the stack arguments that directly follow the system call. */
  int args[3];
  int *sys_call = f->esp;
  char *buffer;
  int size;
  void *buffer_page_ptr;

  switch (*sys_call) {

  case SYS_HALT:
    /*"Halts" the system, shutting it down*/
    shutdown_power_off(); /*pintos/src/devices/shutdown.c*/
    break;

  case SYS_EXIT:
    // get the arguments
    get_arguments(f, &args[0], 1);
    // exit the program
    exit(args[0]);
    break;

  case SYS_EXEC:
    /*TODO: SYSCALL EXEC HANDLER*/
    /*Creates a new process and runs the executable whose name is given in cmd_line, passing any given arguments, and returns the new process's program id (pid).
    Must return pid -1, which otherwise should not be a valid pid, if the program cannot load or run for any reason.
    Thus, the parent process cannot return from the exec until it knows whether the child process successfully loaded its executable.
    If you correctly implemented the synchronization requirement of the previous project, you have already done this.*/
    break;

  case SYS_WAIT:
    /*TODO SYSCALL WAIT HANDLER*/
    /*Waits for a child process pid and retrieves the child's exit status.

    If pid is still alive, waits until it terminates. Then, returns the status that pid passed to exit. If pid did not call exit(), but was terminated by the kernel (e.g. killed due to an exception), wait(pid) must return -1. It is perfectly legal for a parent process to wait for child processes that have already terminated by the time the parent calls wait, but the kernel must still allow the parent to retrieve its child's exit status, or learn that the child was terminated by the kernel.

    wait must fail and return -1 immediately if any of the following conditions is true:

    pid does not refer to a direct child of the calling process. pid is a direct child of the calling process if and only if the calling process received pid as a return value from a successful call to exec. Note that children are not inherited: if A spawns child B and B spawns child process C, then A cannot wait for C, even if B is dead. A call to wait(C) by process A must fail. Similarly, orphaned processes are not assigned to a new parent if their parent process exits before they do.
    The process that calls wait has already called wait on pid. That is, a process may wait for any given child at most once.

    Processes may spawn any number of children, wait for them in any order, and may even exit without having waited for some or all of their children. Your design should consider all the ways in which waits can occur. All of a process's resources, including its struct thread, must be freed whether its parent ever waits for it or not, and regardless of whether the child exits before or after its parent.

    You must ensure that Pintos does not terminate until the initial process exits. The supplied Pintos code tries to do this by calling process_wait() (in "userprog/process.c") from main() (in "threads/init.c"). We suggest that you implement process_wait() according to the comment at the top of the function and then implement the wait system call in terms of process_wait().

    Implementing this system call requires considerably more work than any of the rest.*/
    break;

  case SYS_CREATE:
    /*TODO SYSCALL CREATE HANDLER*/
    break;

  case SYS_REMOVE:
    /*TODO SYSCALL REMOVE HANDLER*/
    break;

  case SYS_OPEN:
    /*TODO SYSCALL OPEN HANDLER*/
    break;

  case SYS_FILESIZE:
    /*TODO SYSCALL FILESIZE HANDLER*/
    break;
  
  case SYS_READ:
    // get arguments
    get_arguments(f, &args[0], 3);
    // variable for holding buffer
    buffer = (char *)args[1];
    // hold size of buffer
    size = args[2];
    // check that each byte of the buffer is valid in memory
    for (int i = 0; i < size; i++) {
      check_address((const void *)buffer);
      buffer++;
    }

    // get the page directory page from memory for the buffer
    buffer_page_ptr =
        pagedir_get_page(thread_current()->pagedir, (const void *)args[1]);
    // checking for valid buffer
    if (buffer_page_ptr == NULL) {
      exit(-1);
    }

    // assign page ptr to buffer
    args[1] = (int)buffer_page_ptr;
    // read from a fd into a buffer and hold
    // the bytes read in return register
    f->eax = read(args[0], (void *)args[1], (unsigned)args[2]);

    break;

  case SYS_WRITE:
    get_arguments(f, &args[0], 3);
    // variable for holding buffer
    buffer = (char *)args[1];
    // hold size of buffer
    size = args[2];
    // check that each byte of the buffer is valid in memory
    for (int i = 0; i < size; i++) {
      check_address((const void *)buffer);
      buffer++;
    }

    // get the page directory page from memory for the buffer
    buffer_page_ptr =
        pagedir_get_page(thread_current()->pagedir, (const void *)args[1]);
    // checking for valid buffer
    if (buffer_page_ptr == NULL) {
      exit(-1);
    }

    // assign page ptr to buffer
    args[1] = (int)buffer_page_ptr;

    // write buffer into a "file" and hold
    // the bytes read in return register
    f->eax = write(args[0], (const void *)args[1], (unsigned)args[2]);

    break;

  case SYS_SEEK:
    /*TODO SYSCALL SEEK HANDLER*/
    break;

  case SYS_TELL:
    /*TODO SYSCALL TELL HANDLER*/
    break;

  case SYS_CLOSE:
    /*TODO SYSCALL CLOSE HANDLER*/
    break;

  default:
    exit(-1);
    break;

    /******************** END PA3 ADDED CODE ********************/
  }
}

/**
 * Exit program
 */
void exit(int status) {
  /*PA3 MODIFIED*/
  /*Print exiting first*/
  printf("%s: exit(%d)\n", thread_current()->name, status);
  /*Retrieve pointer to PCB and set exit_status*/
  struct process_control_block *pcb = thread_current()->pcb;
  if(pcb != NULL){
  pcb->exit_status = status;
  }
  thread_exit();
}

/**
 * Read into a buffer
 */
int read(int fd, void *buffer, unsigned size) {
  int i;
  char *buf = (char *)buffer;
  // use the input fd
  if (fd == 0) {
    // fill the buffer with user input
    for (i = 0; i < (int)size; i++) {
      buf[i] = input_getc();
    }

    return i;
  }

  return 0;
}

/**
 * Write from a buffer
 */
int write(int fd, const void *buffer, unsigned size) {
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return 0;
}
/******************** PA2 ADDED CODE ********************/
/* Determine if the given address is valid in user memory */
void check_address(const void *check_ptr) {
  /* If the address is not within the bounds of user memory or equal
     to null, exit the program with a status of -1.
   */
  if (!is_user_vaddr(check_ptr) || check_ptr == NULL ||
      check_ptr < (void *)0x08048000) {
    /* Terminate the program */
    exit(-1);
  }
}

/* Get the arguments off the stack and store them in a pointer array.
Mainly so pointer manipulation doesn't have to be written for every
system call. */
void get_arguments(struct intr_frame *f, int *args, int num_args) {
  int *arg_ptr;
  for (int i = 0; i < num_args; i++) {
    arg_ptr = (int *)f->esp + i + 1;
    check_address((const void *)arg_ptr);
    args[i] = *arg_ptr;
  }
}
/******************** END PA2 ADDED CODE ********************/
