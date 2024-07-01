#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "utils.h"

void die(const char *s) {
      //refresh screen on exit
  write(STDOUT_FILENO, "\x1b[2j", 4);
  write(STDOUT_FILENO, "\x1b[h", 3);

  perror(s);
  exit(1);
}
