#include <debug.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char *argv[]) {
  // debug_backtrace();
  exit((int)argv[1]);
  // exit(0);
  // printf ("After exit");
  return 0;
}
