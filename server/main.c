#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <bits/pthreadtypes.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "common/constants.h"
#include "common/io.h"
#include "operations.h"


int session_id = 0;
int waiting_sessions = 0;
pthread_mutex_t produce_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t produce_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t consume_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t consume_cond = PTHREAD_COND_INITIALIZER;
struct client prod_cons_buffer[10];

void produce(struct client* client){
  pthread_mutex_lock(&produce_mutex);
  while(waiting_sessions== 10){
    pthread_cond_wait(&produce_cond, &produce_mutex);
  }
  memcpy(&prod_cons_buffer[waiting_sessions], client, sizeof(struct client));
  waiting_sessions++;
  pthread_cond_signal(&consume_cond);
  pthread_mutex_unlock(&produce_mutex);
}

struct client* consume(){
  pthread_mutex_lock(&consume_mutex);
  while(waiting_sessions==0){
    pthread_cond_wait(&consume_cond, &consume_mutex);
  }
  struct client* client = &prod_cons_buffer[waiting_sessions-1];
  waiting_sessions--;
  client->session_id = session_id++;
  pthread_cond_signal(&produce_cond);
  pthread_mutex_unlock(&consume_mutex);
  if((client->fdreq=open(client->req_path, O_RDONLY))==-1){
      fprintf(stderr, "Failed to open request pipe\n");
  }
  
  if((client->fdresp=open(client->res_path, O_WRONLY))==-1){
    fprintf(stderr, "Failed to open response pipe\n");
  }
  if(write(client->fdresp, &client->session_id, sizeof(int))==-1){
    fprintf(stderr, "Failed to write to pipe\n");
  }
  return client;
}


int session(struct client* client){
   int in_fd = client->fdreq;
   int out_fd = client->fdresp;
   char command;
   unsigned int event_id;
   int ret;
   size_t num_rows, num_columns, num_coords;
   size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];
   while (1) {    
    if(read(in_fd, &command, 1)==-1){
      fprintf(stderr, "Failed to read command\n");
      command = '2';
    }

    switch (command) {
      case '2':
        if(close(in_fd)==-1)
          fprintf(stderr, "Failed to close request pipe\n");

        if(close(out_fd)==-1)
          fprintf(stderr, "Failed to close response pipe\n");
        free(client);
        return 0;

      case '3':
        if(read(in_fd, &event_id, sizeof(int))==-1){
          fprintf(stderr, "Failed to read event_id\n");
          continue;
        }
        if(read(in_fd, &num_rows, sizeof(size_t))==-1){
          fprintf(stderr, "Failed to read num_rows\n");
          continue;
        }
        if(read(in_fd, &num_columns, sizeof(size_t))==-1){
          fprintf(stderr, "Failed to read num_columns\n");
          continue;
        }

        ret = ems_create(event_id, num_rows, num_columns);
        if(write(out_fd, &ret, sizeof(int))==-1)
          fprintf(stderr, "Failed to write response\n");

        break;

      case '4':
        if(read(in_fd, &event_id, sizeof(int))==-1){
          fprintf(stderr, "Failed to read event_id\n");
          continue;
        }
        if(read(in_fd, &num_coords, sizeof(size_t))==-1){
          fprintf(stderr, "Failed to read num_coords\n");
          continue;
        }
        if(read(in_fd, xs, sizeof(size_t)*num_coords)==-1){
          fprintf(stderr, "Failed to read xs\n");
          continue;
        }
        if(read(in_fd, ys, sizeof(size_t)*num_coords)==-1){
          fprintf(stderr, "Failed to read ys\n");
          continue;
        }

        ret = ems_reserve(event_id, num_coords, xs, ys);
        if(write(out_fd, &ret, sizeof(int))==-1)
          fprintf(stderr, "Failed to write response\n");
        
        break;

      case '5':
        if(read(in_fd, &event_id, sizeof(int))==-1){
          fprintf(stderr, "Failed to read event_id\n");
          continue;
        }

        ret=ems_show(out_fd, event_id);
        if(write(out_fd, &ret, sizeof(int))==-1)
          fprintf(stderr, "Failed to write response\n");
        
        break;

      case '6':
        ret = ems_list_events(out_fd);
        if(write(out_fd, &ret, sizeof(int))==-1)
          fprintf(stderr, "Failed to write response\n");
        
        break;
    }
  }
}


void proc_clients(void*){
  while(1){
    struct client* client = consume();
    session(client);
  }
}


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

  if(mkfifo(pipe_path, 0777) < 0){
    fprintf(stderr, "Failed to create pipe\n");
    return 1;
  }

  if((main_pipe_fd = open(pipe_path, O_RDONLY))<0){
    fprintf(stderr, "Failed to open pipe\n");
    return 1;
  }

  pthread_t thread[MAX_SESSION_COUNT];
  for(int i=0; i<MAX_SESSION_COUNT; i++){
    if (pthread_create(&thread[i],NULL, (void*)proc_clients, NULL) != 0) {
      fprintf(stderr, "Failed to create thread\n");
      return 1;
    }
  }

  while (1) {
    char code;
    struct client* new_client= malloc(sizeof(struct client));
    char *buffer = malloc(40*sizeof(char));
    //read log in request from pipe
    if(read(main_pipe_fd, &code, 1)==-1){
      fprintf(stderr, "Failed to read from pipe\n");
      continue;
    }
    if(read(main_pipe_fd, buffer , 40)==-1 ){
      fprintf(stderr, "Failed to read from pipe\n");
      free(new_client);
      continue;
    }

    strcpy(new_client->req_path, buffer);
    if (read(main_pipe_fd,buffer, 40)==-1){
      fprintf(stderr, "Failed to read from pipe\n");
      free(new_client);
      continue;
    }
    
    strcpy(new_client->res_path, buffer);
    printf("%s\n",new_client->req_path);
    produce(new_client);  
  }

 for(int i=0; i<MAX_SESSION_COUNT; i++){
    if(pthread_join(thread[i], NULL)!=0){
      fprintf(stderr, "Failed to join thread\n");
      return 1;
    }    
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