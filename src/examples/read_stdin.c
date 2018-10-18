#include "threads/malloc.h"
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv) {
  char *buffer[4096];
  // FIXME: CHANGE TO STATIC BUFFER?
  // buffer = (char *)malloc(10);
  printf("%s\n", "in read");
  // read(0, &buffer, sizeof(buffer));
  // free(buffer);

  return EXIT_SUCCESS;
}
