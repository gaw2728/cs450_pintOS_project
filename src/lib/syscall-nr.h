#ifndef __LIB_SYSCALL_NR_H
#define __LIB_SYSCALL_NR_H

/* System call numbers. */
enum 
  {
    /* Projects 2 and later. */
    /*0*/SYS_HALT,                   /* Halt the operating system. */
    /*1*/SYS_EXIT,                   /* Terminate this process. */
    /*2*/SYS_EXEC,                   /* Start another process. */
    /*3*/SYS_WAIT,                   /* Wait for a child process to die. */
    /*4*/SYS_CREATE,                 /* Create a file. */
    /*5*/SYS_REMOVE,                 /* Delete a file. */
    /*6*/SYS_OPEN,                   /* Open a file. */
    /*7*/SYS_FILESIZE,               /* Obtain a file's size. */
    /*8*/SYS_READ,                   /* Read from a file. */
    /*9*/SYS_WRITE,                  /* Write to a file. */
    /*10*/SYS_SEEK,                   /* Change position in a file. */
    /*11*/SYS_TELL,                   /* Report current position in a file. */
    /*12*/SYS_CLOSE,                  /* Close a file. */

    /* Project 3 and optionally project 4. */
    /*13*/SYS_MMAP,                   /* Map a file into memory. */
    /*14*/SYS_MUNMAP,                 /* Remove a memory mapping. */

    /* Project 4 only. */
    /*15*/SYS_CHDIR,                  /* Change the current directory. */
    /*16*/SYS_MKDIR,                  /* Create a directory. */
    /*17*/SYS_READDIR,                /* Reads a directory entry. */
    /*18*/SYS_ISDIR,                  /* Tests if a fd represents a directory. */
    /*19*/SYS_INUMBER                 /* Returns the inode number for a fd. */
  };

#endif /* lib/syscall-nr.h */
