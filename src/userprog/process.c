#include "userprog/process.h"
#include "userprog/syscall.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include <list.h>

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

/*============================= PA2 ADDED CODE =============================*/
/* PA2 added helper function to keep code clean and push program args to
the stack */
static void push_args (const char *[], int cnt, void **esp);
/*=========================== END PA2 ADDED CODE ===========================*/

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created.

   PA2 VARIABLE CHANGES:
   1)"file_name" (original) misleading for calls to finctions that pass
   arguments along with the name of the binary to be executed.
   Therefore "file_name" changed to "args_line"

   2)"fn_copy" has been changed to hold a copy of all arguments passed
   and so has been changed to "args_copy"*/
pid_t
process_execute (const char *args_line)
{
  char *args_copy = NULL; //PA2 CHANGED
  char *file_name = NULL; //PA2 ADDED
  char *save_ptr = NULL;  //PA2 ADDED
  tid_t tid;

  struct process_control_block *new_pcb = NULL; //PA3 ADDED

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  args_copy = palloc_get_page (0);
  if (args_copy == NULL)
    return TID_ERROR;
  strlcpy (args_copy, args_line, PGSIZE);

  /*============================= PA2 ADDED CODE =============================*/
  /*Strip away the name of the executable*/
  file_name = palloc_get_page (0);
  if (file_name == NULL)
    return TID_ERROR;
  strlcpy (file_name, args_line, PGSIZE);
  file_name = strtok_r(file_name, " ", &save_ptr);
  /*=========================== END PA2 ADDED CODE ===========================*/

  /*============================= PA3 ADDED CODE =============================*/
  new_pcb = palloc_get_page(0);
  if (new_pcb == NULL) {
    if(args_copy) palloc_free_page (args_copy);
    if(file_name) palloc_free_page (file_name);
    if(new_pcb) palloc_free_page (new_pcb);
    return PID_ERROR;
  }

  /*Start setting up the process_control_block for the new process*/
  new_pcb->pid = PID_INIT;
  new_pcb->exit_status = -1;
  new_pcb->cmd_line = args_copy;
  new_pcb->parent = thread_current();
  new_pcb->parent_waiting = false;
  new_pcb->child_exit = false;
  new_pcb->orphan_flag = false;

  sema_init(&new_pcb->process_init_sema, 0);
  sema_init(&new_pcb->process_wait_sema, 0);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, new_pcb);

  if (tid == TID_ERROR) {
    if(args_copy) palloc_free_page (args_copy);
    if(file_name) palloc_free_page (file_name);
    if(new_pcb) palloc_free_page (new_pcb);
    return PID_ERROR;
  }

  /*Wait for new process initialization to be complete in start_process*/
  sema_down(&new_pcb->process_init_sema);
  if(args_copy) {
    palloc_free_page (args_copy);
  }

  /*If successfully made it here, then there is a new child process*/
  if(new_pcb->pid >= 0) {
    list_push_back (&(thread_current()->child_list), &(new_pcb->child_elem));
  }

  palloc_free_page (file_name);

  return new_pcb->pid;
  /*=========================== END PA3 ADDED CODE ===========================*/
}

/* A thread function that loads a user process and starts it
  running. */
static void
start_process (void *new_pcb_)
{
  /*============================= PA3 ADDED CODE =============================*/
  struct thread *cur = thread_current(); /*pnt to the current thread*/
  struct process_control_block *pcb_ptr = new_pcb_; /*ptr to passed pcb*/
  char *file_name = (char *) pcb_ptr->cmd_line; /*string passed through pcb*/
  /*=========================== END PA3 ADDED CODE ===========================*/

  /*token and save_ptr are required for strtok_r()*/
  char *token = NULL;     //PA2 ADDED
  char *save_ptr = NULL;  //PA2 ADDED
  int argc = 0;           //PA2 ADDED
  struct intr_frame if_;
  bool success = false;

  /*============================= PA2 ADDED CODE =============================*/
  /* Break apart the passed "file_name" parameters into individual tokens.
     argv[0] should always be the program to be executed. */
  /* GEFF NOTE: WORKS FOR PA3 AND DOES NOT NEED TO BE CHANGED*/
  const char **argv = (const char**) palloc_get_page(0);
  if (argv == NULL){
    goto loadfail;
  }
  for (token = strtok_r(file_name, " ", &save_ptr); token != NULL;
      token = strtok_r(NULL, " ", &save_ptr))
  {
    argv[argc++] = token;
  }
/*=========================== END PA2 ADDED CODE ===========================*/

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (argv[0], &if_.eip, &if_.esp);

  /*============================= PA3 ADDED CODE =============================*/
  if (success) {
    /*Push arguments to the stack*/
    push_args (argv, argc, &if_.esp);
  }
  palloc_free_page (argv);

  /*Process, in theory, successfully created, need to assign PID
  Which in PintOS 1 to 1 mapping is the TID*/
  if(success){
    pcb_ptr->pid = (pid_t)(cur->tid);
  }
  else {
loadfail:
    pcb_ptr->pid = PID_ERROR;
  }
  /*Assign the current thread's pcb to the created pcb*/
  cur->pcb = pcb_ptr;

  /*wait for the process to be initialized*/
  sema_up(&pcb_ptr->process_init_sema);
  if(!success){
    sys_exit(-1);
  }
  /*=========================== END PA3 ADDED CODE ===========================*/

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid)
{
  /*=========================== PA3 MODIFIED CODE ============================*/
  /*retrieve a pointer to the calling thread*/
  struct thread *cur_thread = thread_current();
  /*retrieve a pointer to the calling thread's child list*/
  struct list *cur_child_list = &(cur_thread->child_list);
  /*used to search the cur_child_list*/
  struct list_elem *child_member = NULL;
  /*if a correct child is found, this will hold it's pcb*/
  struct process_control_block *child_pcb = NULL;

  /*Seach through the child list of cur_thread and see is a child exists of
  child_tid. Again, mapping is 1 to 1 so tid == pid*/
  if (!list_empty(cur_child_list)) {
    for (child_member = list_front(cur_child_list); child_member != list_end(cur_child_list); child_member = list_next(child_member)) {
      /*Get the current child's pcb and check it.*/
      struct process_control_block *pcb = list_entry(
          child_member, struct process_control_block, child_elem);
      if(pcb->pid == child_tid) { 
        /*if child is found, assign and break search*/
        child_pcb = pcb;
        break;
      }
    }
  }

  /*Handle project doc exceptions:
  1) Child is not a direct child of the caller, and so child_pcb is still NULL
  2) child is already being waited upon
  Both of these conditions cause an automatic return of -1*/
  if (child_pcb == NULL) {
    return -1; 
  }
  else if (child_pcb->parent_waiting == true) {
    return -1;
  }
  else {
    child_pcb->parent_waiting = true;
  }
  /*If the child has yet to exit block on the process wait semaphore*/
  if (! child_pcb->child_exit) {
    sema_down(&child_pcb->process_wait_sema);
  }

  /*At this point it is assumed the child has exited*/
  ASSERT (child_pcb->child_exit == true);

  /*Remove this child from caller's child list*/
  ASSERT (child_member != NULL);
  list_remove (child_member);

  /*By project doc, exit status must always be returned*/
  int exit_status = child_pcb->exit_status;

  /*Free memory of child_pcb*/
  palloc_free_page(child_pcb);

  return exit_status;
  /*=========================== END PA3 ADDED CODE ===========================*/
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;
  /*=========================== PA3 MODIFIED CODE ============================*/
  /*FILE DISCRIPTOR FREEING CAUSING TESTS TO FAIL.*/
  /*Handle closing of all file resources*/
  struct list_elem *e;
  struct open_file *cur_file;

  // navigate through the list of open file descriptors
  for (e = list_begin(&cur->file_list); e != list_end(&cur->file_list);
       e = list_next(e)) {
    cur_file = list_entry(e, struct open_file, file_elem);
    close(cur_file->fd);
  }

  /*If this process had children all of the child resources need to be
  cleaned up*/
  struct list *child_list = &cur->child_list;
  while (!list_empty(child_list)) {
    struct list_elem *cur_child = list_pop_front (child_list);
    struct process_control_block *pcb;
    pcb = list_entry(cur_child, struct process_control_block, child_elem);
    /*If the process has exited it can be freed*/
    if (pcb->child_exit == true) {
      palloc_free_page (pcb);
    } else {
    /*child is orphaned*/
      pcb->orphan_flag = true;
      pcb->parent = NULL;
    }
  }

  /*With children handled it's time to exit the current process.
  Also check the current processes status as an orphan for resource
  freeing. signal to waiting processes that this process is now exiting.*/
  cur->pcb->child_exit = true;
  bool cur_orphan = cur->pcb->orphan_flag;
  sema_up (&cur->pcb->process_wait_sema);

  /*if orphan status confirmed than free process control block*/
  if (cur_orphan) {
    palloc_free_page (&cur->pcb);
  }
  /*=========================== END PA3 ADDED CODE ===========================*/

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL)
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp)
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL)
    goto done;
  process_activate ();

  /* Open executable file. */
  file = filesys_open (file_name);
  if (file == NULL)
    {
      printf ("load: %s: open failed\n", file_name);
      goto done;
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024)
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done;
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++)
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type)
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file))
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file)
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
    return false;

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file))
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz)
    return false;

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;

  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0)
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false;
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable))
        {
          palloc_free_page (kpage);
          return false;
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp)
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL)
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        *esp = PHYS_BASE;
      else
        palloc_free_page (kpage);
    }
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

/*============================= PA2 ADDED CODE =============================*/
/* push_args
   ARGS:
    char *argv_tokens: arguments for the established process
    int argc: the number of arguments in argv_tokens
    void **esp: The stack pointer to the process' stack in memory

   push_args takes in the argv, argc, and stack pointer for a process and
   sets up the process stack by the following convention.

   0xbffffffc  argv[3][...]  "bar\0"   char[4]    highest ^  pushed |
   0xbffffff8  argv[2][...]  "foo\0"   char[4]            |         |
   0xbffffff5  argv[1][...]  "-l\0"  char[3]              |         |
   0xbfffffed  argv[0][...]  "/bin/ls\0"   char[8]        |         |
   0xbfffffec  word-align  0   uint8_t                    |         |
   0xbfffffe8  argv[4]   0   char *                       |         |
   0xbfffffe4  argv[3]   0xbffffffc  char *               |         |
   0xbfffffe0  argv[2]   0xbffffff8  char *               |         |
   0xbfffffdc  argv[1]   0xbffffff5  char *               |         |
   0xbfffffd8  argv[0]   0xbfffffed  char *               |         |
   0xbfffffd4  argv      0xbfffffd8  char **              |         |
   0xbfffffd0  argc  4   int                              |         |
   0xbfffffcc  return address  0   void (*) ()     lowest |         V  */
static void
push_args (const char *argv_tokens[], int argc, void **esp)
{
  /* There had better be 0 or more arguments*/
  ASSERT(argc >= 0);
  ASSERT((((argc * 2) + 5) * 32) < PGSIZE);
  int i = 0;             /* Utility variable for loops*/
  int elem_length = 0;    /* Length of an argv element */
  /* pushing the argv[] members */
  void* argv_addr[argc];
  for (i = 0; i < argc; i++) {
    elem_length = strlen(argv_tokens[i]) + 1;
    *esp -= elem_length;
    memcpy(*esp, argv_tokens[i], elem_length);
    argv_addr[i] = *esp;
  }

  /* push the word-align 
  Since 4 bytes = 32 bits the stack pointer should be
  moved 4 - (address%4) bytes to account for uneven
  argv actuals. (They don't line-up regularly)*/
  *esp -= 4 - (int)esp % 4;

  /* push the required null pointer */
  *esp -= 4;
  *((uint32_t*) *esp) = 0;

  /* pushing the pointers to the argv elements */
  for (i = argc - 1; i >= 0; i--) {
    *esp -= 4;
    *((void**) *esp) = argv_addr[i];
  }

  /* push **argv aka. the pointer to the argv vector */
  *esp -= 4;
  *((void**) *esp) = (*esp + 4);

  /* push argc */
  *esp -= 4;
  *((int*) *esp) = argc;

  /* push return address */
  *esp -= 4;
  *((int*) *esp) = 0;

}
/*=========================== END PA2 ADDED CODE ===========================*/
