#include "userprog/syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>

static void syscall_handler(struct intr_frame *);

void syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler(struct intr_frame *f UNUSED) {
  // TODO: REMOVE TEST
  int *sys_num;
  int status;

  sys_num = (int *)f->esp + 0;

  printf("%0x\n", *sys_num);

  // intr_dump_frame(f);
  // printf("Vector number: %d\n", f->vec_no);
  // printf("%d\n", SYS_TELL);
  switch (*sys_num) {
  case SYS_EXIT:
    // memcpy(&status, f->esp + 4, sizeof(status));
    // printf("%d\n", status);
    // printf("%d\n", *((char *)(f->esp + 4)));
    // TODO Need to convert f->esp + 4 to an int so we can pass it to exit()
    // exit((int *)f->esp[1]);
    break;
  case SYS_READ:
    // read();
    break;
  case SYS_WRITE:
    // write();
    break;
  default:
    break;
  }
  printf("system call!\n");
  thread_exit();
}
