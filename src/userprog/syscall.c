#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/vaddr.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
/******************** PA2 ADDED CODE ********************/
void get_stack_arguments (struct intr_frame *f, int * args, int num_of_args);
/******************** END PA2 ADDED CODE ********************/

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
   /******************** PA2 ADDED CODE ********************/
	/* Holds the stack arguments that directly follow the system call. */
	int args[3];
	char *sp = f->esp;
	// The switch case will probably look like this switch (*(int*)sp)
  	printf ("system call!\n");
  	//args passed for a printf are an int, buffer, and size
  	printf("The system call number is %d.\n", (int)*sp);
  	printf("The enum for a write is %d.\n", SYS_WRITE);

  	get_stack_arguments(f, &args[0], 3);
  	printf("The file descriptor passed was %d.\n", args[0]);
  	printf("The string passed to printf was \"%s\".\n",(char*) args[1]);
  	printf("The number of bytes to write that was passed was %d.\n", args[2]);
  thread_exit ();
  /******************** END PA2 ADDED CODE ********************/
}
/******************** PA2 ADDED CODE ********************/

/* Code inspired by GitHub Repo created by ryantimwilson (full link in Design2.txt).
   Get up to three arguments from a programs stack (they directly follow the system
   call argument). */
void get_stack_arguments (struct intr_frame *f, int *args, int num_of_args)
{
  int i;
  int *ptr;
  for (i = 0; i < num_of_args; i++)
    {
      ptr = (int *) f->esp + i + 1;
      args[i] = *ptr;
    }
}
/******************** END PA2 ADDED CODE ********************/
