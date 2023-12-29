#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include <asm-generic/fcntl.h>

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    return 1;
  }
  int main_pipe_fd;
  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  char* pipe_path = argv[1];
  if (strlen(pipe_path) > 256) {
    fprintf(stderr, "Pipe path too long\n");
    return 1;
  }
  if(mkfifo(pipe_path, 0777)<0){
    fprintf(stderr, "Failed to create pipe\n");
    return 1;
  }
  if((main_pipe_fd = open(pipe_path, O_RDWR))<0){
    fprintf(stderr, "Failed to open pipe\n");
    return 1;
  }

  while (1) {
    //TODO: Read from pipe
    //TODO: Write new client to the producer-consumer buffer
  }
  if (close(main_pipe_fd) < 0) {
    fprintf(stderr, "Failed to close pipe\n");
    return 1;
  }

  if(unlink(pipe_path)<0){
    fprintf(stderr, "Failed to unlink pipe\n");
    return 1;
  }
  ems_terminate();
}