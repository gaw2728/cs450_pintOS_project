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
struct open_file {
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
bool create(const char *file, unsigned initial_size);
int filesize (int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
bool remove (const char *file);
unsigned tell(int fd);
struct file *get_file(int fd);
void mem_access_failure(void); // called to release lock and exit
int add_file_to_process (struct file *f);
void close_file_from_process (int fd);
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

  /*It is not enough to check that the stack pointer is a valid
  virtual address pointer. One must check that the virtual
  address pointer refers to a valid page for the process'
  memory.*/
  user_to_kernel_ptr((const void *)f->esp);

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
    get_arguments(f, &args[0], 2);
    args[0] = user_to_kernel_ptr((const void *)args[0]);
    f->eax = create((const char *)args[0], (unsigned)args[1]);
    break;

  case SYS_REMOVE:
    get_arguments(f, &args[0], 1);
    //check_valid_string((const void *) args[0]);
    args[0] = user_to_kernel_ptr((const void *) args[0]);
    f->eax = remove((const char *) args[0]);
    break;

  case SYS_OPEN:
    /* Opens the file called file. Returns a nonnegative integer handle called a
    "file descriptor" (fd), or -1 if the file could not be opened.*/
    get_arguments(f, &args[0], 1); //process args
    args[0] = user_to_kernel_ptr((const void *)args[0]); //assign to pointer
    f->eax = open((const char *)args[0]);
    break;

  case SYS_FILESIZE:
  /*TODO Elliott: SYSCALL FILESIZE HANDLER, MAKE SURE IT WORKS*/
    get_arguments(f, &args[0], 1);
    f->eax = filesize((int)args[0]); //TODO Do I need to do this?
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
    /*TODO Elliott: SYSCALL SEEK HANDLER, MAKE SURE IT WORKS*/
    get_arguments(f, &args[0], 2);
    //args[0] = fd, args[1] = position to seek to
    seek((int)args[0], (unsigned)args[1]);
    break;

  case SYS_TELL:
    /*TODO Elliott: SYSCALL TELL HANDLER, MAKE SURE IT WORKS*/
    get_arguments(f, &args[0], 1);
    f->eax = tell((int)args[0]); //args[0] = fd //TODO Do I need to set f->eax?
    break;

  case SYS_CLOSE:
    get_arguments(f, &args[0], 1);
    close(args[0]);
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
 * Creates a new file in the file system
 */
bool create(const char *file, unsigned initial_size) {
  bool success;

  lock_acquire(&filesys);
  success = filesys_create(file, initial_size);
  lock_release(&filesys);

  return success;
}

int open (const char *file) {
  lock_acquire(&filesys); //get the lock
  struct file *current_file = filesys_open(file); //populate struct
  if (!current_file) {
    lock_release(&filesys);
    return -1; //TODO replace this?
  }
  int fd = add_file_to_process(current_file);
  lock_release(&filesys);
  return fd;

}

bool remove (const char *file)
{
  lock_acquire(&filesys);
  bool success = filesys_remove(file);
  lock_release(&filesys);
  return success;
}

/**
* Returns the size of a file as the number of bytes
*/
int filesize(int fd) {
  struct file *f;
  int num_bytes;

  lock_acquire(&filesys);

  // get the correct file and check if it's valid (in file system)
  if (!(f = get_file(fd))) {
    lock_release(&filesys);
    return -1;
  }
  //gets the size of a file in bytes
  num_bytes = file_length(f);

  lock_release(&filesys);

  return num_bytes;
}

/**
 * Read into a buffer
 */
int read(int fd, void *buffer, unsigned size) {
  int i;
  int bytes_read;
  char *buf;
  struct file *f;

  // use the input fd
  if (fd == 0) {
    buf = (char *)buffer;
    // fill the buffer with user input
    for (i = 0; i < (int)size; i++) {
      buf[i] = input_getc();
    }
    return i;
  }

  lock_acquire(&filesys);

  // get the correct file and check if it's valid (in file system)
  if (!(f = get_file(fd))) {
    lock_release(&filesys);
    return -1;
  }

  // read from the given file and into a buffer
  // and record the number of bytes read
  bytes_read = file_read(f, buffer, size);

  lock_release(&filesys);

  return bytes_read;
}

/**
 * Write from a buffer
 */
int write(int fd, const void *buffer, unsigned size) {
  int bytes_written;
  struct file *f;

  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }

  lock_acquire(&filesys);

  // get the correct file and check if it's valid (in file system)
  if (!(f = get_file(fd))) {
    lock_release(&filesys);
    return -1;
  }

  // write to the given file from a buffer
  // and record the number of bytes written
  bytes_written = file_write(f, buffer, size);

  lock_release(&filesys);

  return bytes_written;
}

/**
* Sets the current position in fd to position bytes from the start of fd
* (Position of 0 is the file's start)
*/
void seek(int fd, unsigned position) {
  struct file *f;

  lock_acquire(&filesys);

  // get the correct file and check if it's valid (in file system)
  if (!(f = get_file(fd))) {
    lock_release(&filesys);
  } else {
    //sets the current position in fd to position bytes from the start of fd
    file_seek(f, position);
    lock_release(&filesys);
  }
}

/**
* Returns the current position in fd as a byte offset from the
* start of the file.
*/
unsigned tell(int fd) {
  struct file *f;
  unsigned position;

  lock_acquire(&filesys);

  // get the correct file and check if it's valid (in file system)
  if (!(f = get_file(fd))) {
    lock_release(&filesys);
    return -1;
  }
  //sets the current position in fd to position bytes from the start of fd
  position = file_tell(f);
  lock_release(&filesys);

  return position;
}

void close (int fd)
{
  lock_acquire(&filesys);
  close_file_from_process(fd);
  lock_release(&filesys);
}

/*<==================== HELPER FUNCTIONS ====================> */

/**
 * Retrieve the file associated with the given fd.
 */
struct file *get_file(int fd) {
  struct thread *t_cur = thread_current();
  struct list_elem *e;

  // navigate through the list of open file descriptors
  for (e = list_begin(&t_cur->file_list); e != list_end(&t_cur->file_list);
       e = list_next(e)) {
    struct open_file *f = list_entry(e, struct open_file, elem);

    // check and return the matching file descriptor
    if (fd == f->fd)
      return f->file;
  }

  return NULL;
}

/* populates the file struct and returns the file descriptor */
int add_file_to_process (struct file *f)
{
  struct open_file *pf = malloc(sizeof(struct open_file));
  pf->file = f;
  pf->fd = thread_current()->fd;
  thread_current()->fd++;
  list_push_back(&thread_current()->file_list, &pf->elem);
  return pf->fd;
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


void close_file_from_process (int fd)
{
  struct thread *t = thread_current();
  struct list_elem *next, *e = list_begin(&t->file_list);

  while (e != list_end (&t->file_list)) {
      next = list_next(e);
      struct open_file *pf = list_entry (e, struct open_file, elem);
      if (fd == pf->fd || fd == -1) {
        file_close(pf->file);
        list_remove(&pf->elem);
        free(pf);
        if (fd != -1) {
          return;
        }
      }
      e = next;
  }
}

/******************** END PA3 ADDED CODE ********************/
