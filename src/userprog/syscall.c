#include "userprog/syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
	char *sp = f->esp;
  	printf ("system call!\n");
  	//args passed for a printf are an int, buffer, and size
  	printf("The system call number is %d.\n", (int)*sp);
  thread_exit ();
}
