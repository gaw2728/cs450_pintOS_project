#include "userprog/syscall.h"
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "list.h"
#include "process.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include <console.h>
#include <stdio.h>
#include <syscall-nr.h>

/********************** PA3 ADDED CODE **********************/
/*This lock is for protecting file system access*/
struct lock filesys;

/********************** PA3 ADDED CODE **********************/
// holds general information about a file
// TODO: CONSIDER REFACTORING FIELD NAMES
struct process_file {
  struct file *file;     // filesys file pointer
  int fd;                // file descriptor
  struct list_elem elem; // represents this file in the file_list
};

static void syscall_handler(struct intr_frame *);

void get_arguments(struct intr_frame *f, int *args, int num_of_args);
int user_to_kernel_ptr(const void *vaddr);
void check_address(const void *ptr_to_check);
void sys_exit(int status);
pid_t exec(const char *cmd_line);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void mem_access_failure(void); // called to release lock and exit
/******************** END PA3 ADDED CODE ********************/

void syscall_init(void) {
  /********************** PA3 ADDED CODE **********************/
  lock_init(&filesys);
  /******************** END PA3 ADDED CODE ********************/
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
  /*Result currently used in sys_exec & sys_wait*/
  int result;

  check_address((void *)f->esp);

  switch (*sys_call) {

  case SYS_HALT:
    /*"Halts" the system, shutting it down*/
    shutdown_power_off(); /*pintos/src/devices/shutdown.c*/
    break;

  case SYS_EXIT:
    // get the arguments
    get_arguments(f, &args[0], 1);
    // exit the program
    sys_exit(args[0]);
    break;

  case SYS_EXEC:
    get_arguments(f, &args[0], 1);
    args[0] = user_to_kernel_ptr((const void *)args[0]);
    result = exec((const char *)args[0]);
    f->eax = result;
    break;

  case SYS_WAIT:
    /*Below retrieves the pid that the process is waiting on.*/
    get_arguments(f, &args[0], 1);
    int pid = (int)args[0];
    result = (int)process_wait(pid);
    f->eax = (uint32_t)result;
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
      sys_exit(-1);
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
      sys_exit(-1);
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
    // printf("EXIT CALLED IN DEFAULT\n");
    sys_exit(-1);
    break;

    /******************** END PA3 ADDED CODE ********************/
  }
}

/******************** PA3 MODIFIED CODE ********************/

/**
 * Exit program
 */
void sys_exit(int status) {
  /*PA3 MODIFIED*/
  /*Print exiting first*/
  printf("%s: exit(%d)\n", thread_current()->name, status);
  /*Retrieve pointer to PCB and set exit_status*/
  struct process_control_block *pcb = thread_current()->pcb;
  if (pcb != NULL) {
    pcb->exit_status = status;
  }
  thread_exit();
}

/*
  Implementation for exec
*/
pid_t exec(const char *cmd_line) {
  lock_acquire(&filesys);
  pid_t pid = process_execute(cmd_line);
  lock_release(&filesys);
  return pid;
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

/* Determine if the given address is valid in user memory */
void check_address(const void *check_ptr) {
  /* If the address is not within the bounds of user memory or equal
     to null, exit the program with a status of -1.

     is_user_vaddr is PintOS built-in that returns T iff check_ptr < PHYS_BASE
     pointer should not be null
     ask BUCHHOLZ about last param*/
  if (check_ptr == NULL || !is_user_vaddr(check_ptr) ||
      check_ptr < (void *)0x08048000) {
    /* Terminate the program */
    mem_access_failure();
  }
}

/*Called in cases of invalid memory access to free lock if
needed and exit*/
void mem_access_failure(void) {
  if (lock_held_by_current_thread(&filesys)) {
    lock_release(&filesys);
  }
  // printf("EXIT CALLED IN MEM_ACCESS_FAILURE\n");
  sys_exit(-1);
}

/*
  Used in processes like exec to varify that a user pointer is
  good, then translate it into a kernel pointer.
*/
int user_to_kernel_ptr(const void *vaddr) {
  // TODO: Need to check if all bytes within range are correct
  // for strings + buffers
  check_address(vaddr);
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr) {
    mem_access_failure();
  }
  return (int)ptr;
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

/******************** END PA3 ADDED CODE ********************/
