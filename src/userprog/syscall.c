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
void get_stack_arguments(struct intr_frame *f, int *args, int num_of_args);
void check_valid_addr(const void *ptr_to_check);
void exit(int status);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
/******************** END PA2 ADDED CODE ********************/

void syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler(struct intr_frame *f) {
  /******************** PA2 ADDED CODE ********************/
  /* Holds the stack arguments that directly follow the system call. */
  int args[3];
  char *sp = f->esp;
  int *sys_call = f->esp;
  char *buffer;
  int size;
  void *buffer_page_ptr;
  // The switch case will probably look like this switch (*(int*)sp)
  printf("system call!\n");
  // args passed for a printf are an int, buffer, and size
  printf("The system call number is %d.\n", (int)*sp);
  printf("The enum for a write is %d.\n", SYS_WRITE);

  // get_stack_arguments(f, &args[0], 3);
  // printf("The file descriptor passed was %d.\n", args[0]);
  // printf("The string passed to printf was \"%s\".\n", (char *)args[1]);
  // printf("The number of bytes to write that was passed was %d.\n", args[2]);

  switch (*sys_call) {
  case SYS_EXIT:
    get_stack_arguments(f, &args[0], 1);
    thread_current()->exit_status = args[1];
    /*TODO: SET STATUS*/
    printf("%s\n", "EXITING!");
    exit(args[1]);
    break;
    /**/
  case SYS_READ:
    // get arguments
    get_stack_arguments(f, &args[0], 3);
    // variable for holding buffer
    buffer = (char *)args[1];
    // hold size of buffer
    size = args[2];
    printf("%s\n", "1");
    // check that each byte of the buffer is valid in memory
    for (int i = 0; i < size; i++) {
      check_valid_addr((const void *)buffer);
      buffer++;
    }

    // get the page directory page from memory for the buffer
    buffer_page_ptr =
        pagedir_get_page(thread_current()->pagedir, (const void *)args[1]);
    // checking for valid buffer
    if (buffer_page_ptr == NULL) {
      exit(-1);
    }
    printf("%s\n", "2");

    // assign page ptr to buffer
    args[1] = (int)buffer_page_ptr;
    // read from a fd into a buffer and hold
    // the bytes read in return register
    f->eax = read(args[0], (void *)args[1], (unsigned)args[2]);

    printf("%s\n", (char *)args[1]);

    break;
  case SYS_WRITE:
    get_stack_arguments(f, &args[0], 3);
    // variable for holding buffer
    buffer = (char *)args[1];
    // hold size of buffer
    size = args[2];
    printf("%s\n", "4");
    // check that each byte of the buffer is valid in memory
    for (int i = 0; i < size; i++) {
      check_valid_addr((const void *)buffer);
      buffer++;
    }

    // get the page directory page from memory for the buffer
    buffer_page_ptr =
        pagedir_get_page(thread_current()->pagedir, (const void *)args[1]);
    // checking for valid buffer
    if (buffer_page_ptr == NULL) {
      exit(-1);
    }
    printf("%s\n", "5");

    // assign page ptr to buffer
    args[1] = (int)buffer_page_ptr;

    // write buffer into a "file" and hold
    // the bytes read in return register
    f->eax = write(args[0], (const void *)args[1], (unsigned)args[2]);

    break;
  default:
    break;
    // }

    thread_exit();
    /******************** END PA2 ADDED CODE ********************/
  }
}

/**
 * Exit program
 */
void exit(int status) {
  /*TODO: ASK ABOUT STATUS*/
  // process termination message (process_exit) ()if pd!= null
  // hold exit status
  thread_current()->exit_status = status;
  thread_exit();
}

/**
 * Read into a buffer
 */
int read(int fd, void *buffer, unsigned size) {
  printf("%s\n", "3");
  int i;
  char *buf = (char *)buffer;
  printf("%d\n", size);
  // use the input fd
  if (fd == 0) {
    // fill the buffer with user input
    for (i = 0; i < (int)size; i++) {
      buf[i] = input_getc();
      printf("%s\n", "in loop");
    } //
    printf("%s\n", "out loop");
    printf("%s\n", buf);

    return i;
  }

  return 0;
  // TODO: RETURN
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
/* Check to make sure that the given pointer is in user space,
   and is not null. We must exit the program and free its resources should
   any of these conditions be violated. */
void check_valid_addr(const void *ptr_to_check) {
  /* Terminate the program with an exit status of -1 if we are passed
     an argument that is not in the user address space or is null. Also make
     sure that pointer doesn't go beyond the bounds of virtual address space.
   */
  if (!is_user_vaddr(ptr_to_check) || ptr_to_check == NULL ||
      ptr_to_check < (void *)0x08048000) {
    /* Terminate the program and free its resources */
    exit(-1);
  }
}

/* Code inspired by GitHub Repo created by ryantimwilson (full link in
   Design2.txt). Get up to three arguments from a programs stack (they
   directly follow the system call argument). */
void get_stack_arguments(struct intr_frame *f, int *args, int num_of_args) {
  int i;
  int *ptr;
  for (i = 0; i < num_of_args; i++) {
    ptr = (int *)f->esp + i + 1;
    check_valid_addr((const void *)ptr);
    args[i] = *ptr;
  }
}
/******************** END PA2 ADDED CODE ********************/
