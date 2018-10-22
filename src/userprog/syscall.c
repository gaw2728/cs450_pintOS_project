#include "userprog/syscall.h"
#include "devices/input.h"
#include "list.h"
#include "process.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
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
  /******************** PA2 ADDED CODE ********************/
  /* Holds the stack arguments that directly follow the system call. */
  int args[3];
  int *sys_call = f->esp;
  char *buffer;
  int size;
  void *buffer_page_ptr;

  switch (*sys_call) {
  case SYS_EXIT:
    // get the arguments
    get_arguments(f, &args[0], 1);
    // exit the program
    exit(args[0]);
    break;
    /**/
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
  default:
    exit(-1);
    break;
    /******************** END PA2 ADDED CODE ********************/
  }
}

/**
 * Exit program
 */
void exit(int status) {
  /* GEFF'S NOTE: STATUS HERE IS SET PROPERLY.
  THE EXIT MESSAGE PRINTOUT HANDLED IN
  PROCESS.C->PROCESS_EXIT*/
  thread_current()->exit_status = status;
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
