#include "api.h"


int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  //TODO: create pipes and connect to the server
  int fdserver;
  // Write a connection request to the server
  if((fdserver = open(server_pipe_path, O_WRONLY))==-1){
    fprintf(stderr, "Failed to open server pipe\n");
    return 1;
  } 
  if (write(fdserver, 1, sizeof(int)) < 0){
    fprintf(stderr, "Failed to write to server pipe\n");
    return 1;
  }
  if (write(fdserver, req_pipe_path, 40) < 0){
    fprintf(stderr, "Failed to write to server pipe\n");
    return 1;
  }
  if (write(fdserver, resp_pipe_path, 40) < 0){
    fprintf(stderr, "Failed to write to server pipe\n");
    return 1;
  }
   if (close(fdserver)<0){
    fprintf(stderr, "Failed to close server pipe\n");
    return 1;
  }
  if((fdserver = open(server_pipe_path, O_RDONLY))==-1){
    fprintf(stderr, "Failed to open server pipe\n");
    return 1;
  }
  if(read(fdserver, session_id, sizeof(int))<0){
    fprintf(stderr, "Failed to read from server pipe\n");
    return 1;
  }
  if (close(fdserver)<0){
    fprintf(stderr, "Failed to close server pipe\n");
    return 1;
  }
  if(mkfifo(req_pipe_path, 0777)<0){
    fprintf(stderr, "Failed to create request pipe\n");
    return 1;
  }
  if(mkfifo(resp_pipe_path, 0777)<0){
    if(unlink(req_pipe_path)<0){
      fprintf(stderr, "Failed to unlink request pipe\n");
    }
    fprintf(stderr, "Failed to create response pipe\n");
    return 1;
  }
  if ((fres = open (resp_pipe_path, O_WRONLY)) < 0){
    if(unlink(req_pipe_path)<0){
      fprintf(stderr, "Failed to unlink request pipe\n");
    }
    if(unlink(resp_pipe_path)<0){
      fprintf(stderr, "Failed to unlink reponse pipe\n");
    }
    fprintf(stderr, "Failed to open response pipe\n");
    return 1;
  }
  if ((freq = open (req_pipe_path, O_RDONLY)) < 0) {
    if(close(fres)<0){
      fprintf(stderr, "Failed to close response pipe\n");
    }
    if(unlink(req_pipe_path)<0){
      fprintf(stderr, "Failed to unlink request pipe\n");
    }
    if(unlink(resp_pipe_path)<0){
      fprintf(stderr, "Failed to unlink reponse pipe\n");
    }
    fprintf(stderr, "Failed to open request pipe\n");
    return 1;
  }
  this_req_pipe_path = req_pipe_path;
  this_resp_pipe_path = resp_pipe_path;

  return 0;
}

int ems_quit(void) { 
  if (write(freq, 2, sizeof(int)) < 0){
    fprintf(stderr, "Failed to write to request pipe\n");
    return 1;
  }
  if(close(freq)<0){
      fprintf(stderr, "Failed to close request pipe\n");
      return 1;
  }
  if(close(fres)<0){
      fprintf(stderr, "Failed to close response pipe\n");
      return 1;
  }
  if(unlink(this_req_pipe_path)<0){
    fprintf(stderr, "Failed to unlink request pipe\n");
    return 1;
  }
  if(unlink(this_resp_pipe_path)<0){
    fprintf(stderr, "Failed to unlink reponse pipe\n");
    return 1;
  }
 
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  int ret;
  if(write(freq, 3,sizeof(int))<0) {
    fprintf(stderr, "Failed to write command to request pipe\n");
    return 1;
  }
  if(write(freq, &event_id ,sizeof(int))<0) {
    fprintf(stderr, "Failed to write event_id to request pipe\n");
    return 1;
  }
  if(write(freq, &num_rows,sizeof(size_t))<0) {
    fprintf(stderr, "Failed to write num_rows to request pipe\n");
    return 1;
  }
  if(write(freq, &num_cols,sizeof(size_t))<0) {
    fprintf(stderr, "Failed to write num_cols to request pipe\n");
    return 1;
  }
  // descobrir o que fazer com o retorno
  if(read(fres, &ret, sizeof(int))<0) {
    fprintf(stderr, "Failed to read response from response pipe\n");
    return 1;
  }
  else if (ret == 1) {
    fprintf(stderr, "Failed to create event\n");
      return 1;
  }
  else {
    return 0;
  }
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  int ret;
  if(write(freq,4,sizeof(int))<0) {
    fprintf(stderr, "Failed to write command to request pipe\n");
    return 1;
  }
  if(write(freq,&event_id,sizeof(int))<0) {
    fprintf(stderr, "Failed to write event_id to request pipe\n");
    return 1;
  }
  if(write(freq,&num_seats,sizeof(size_t))<0) {
    fprintf(stderr, "Failed to write num_seats to request pipe\n");
    return 1;
  }
  if(write(freq,xs,sizeof(size_t)*num_seats)<0) {
    fprintf(stderr, "Failed to write xs array to request pipe\n");
    return 1;
  }
  if(write(freq,ys,sizeof(size_t)*num_seats)<0) {
    fprintf(stderr, "Failed to write ys array to request pipe\n");
    return 1;
  }
  // descobrir o que fazer com o retorno
  if(read(fres, &ret, sizeof(int))<0){
    fprintf(stderr, "Failed to read response from response pipe\n");
    return 1;
  } else if (ret == 1) {
    fprintf(stderr, "Failed to reserve seats\n");
      return 1;
  }
  else {
    return 0;
  }

}

int ems_show(int out_fd, unsigned int event_id) {
  //TODO: send show request to the server (through the request pipe) and wait for the response (through the response pipe)
  size_t num_rows, num_cols;
  int ret;
  if (write(freq,5,sizeof(int)) < 0) {
    fprintf(stderr, "Failed to write command to request pipe\n");
    return 1;
  }
  if(write(freq, event_id,sizeof(int))<0) {
    fprintf(stderr, "Failed to write event_id to request pipe\n");
    return 1;
  }
  if(read(fres, &ret, sizeof(int))<0) {
    fprintf(stderr, "Failed to read response from response pipe\n");
    return 1;
  }
  if(ret == 1) {
    fprintf(stderr, "Failed to show seats\n");
    return 1;
  }
  if(read(fres, &num_rows, sizeof(size_t))<0) {
    fprintf(stderr, "Failed to read num_rows from response pipe\n");
    return 1;
  }
  if(read(fres, &num_cols, sizeof(size_t))<0) {
    fprintf(stderr, "Failed to read num_cols from response pipe\n");
    return 1;
  }
  unsigned int seats[num_rows][num_cols];
  if(read(fres, seats, sizeof(unsigned int)*num_rows*num_cols)<0) {
    fprintf(stderr, "Failed to read seats array from response pipe\n");
    return 1;
  }
  //adiconar mutex para o file desciptor
  for (size_t i = 1; i <=num_rows; i++) {
    for (size_t j = 1; j <= num_cols; j++) {
      char buffer[16];
      sprintf(buffer, "%u", seats[i][j]);

      if (write(out_fd, buffer ,sizeof(buffer))==-1) {
        perror("Error writing to file");
        return 1;
      }

      if (j <num_cols) {
        if (write(out_fd, " ",1)==-1) {
          perror("Error writing to file descriptor");
          return 1;
        }
      }
    }

    if (write(out_fd, "\n",1)==-1) {
      perror("Error writing to file descriptor");
      return 1;
    }
  }
  return 0;
}
  

int ems_list_events(int out_fd) {
  int ret;
  size_t num_events;
  if (write(freq,6,sizeof(int)) < 0) {
    fprintf(stderr, "Failed to write command to request pipe\n");
    return 1;
  }
  if(read(fres, &ret, sizeof(int))<0) {
    fprintf(stderr, "Failed to read return\n");
  }
  if(ret == 1) {
    fprintf(stderr, "Failed to list events\n");
    return 1;
  }
  if(read(fres, &num_events, sizeof(size_t))<0) {
    fprintf(stderr, "Failed to read num_events\n");
    return 1;
  }
  if(num_events == 0){
    char buffer[] = "No events \n";
    if (write(out_fd, buffer,sizeof(buffer))==-1) {
      fprintf(stderr,"Error writing to file descriptor");
      return 1;
    }
    return 0;
  }
    
  unsigned int events[num_events];
  if(read(fres, &events , sizeof(unsigned int)*num_events)<0)
    return 1;
  for (size_t i = 1; i <=num_events; i++) {
    char buffer[] = "Event: ";
    if (write(out_fd, buffer, sizeof(buffer))) {
       fprintf(stderr,"Error writing to file descriptor");
      return 1;
    }

    char id[16];
    sprintf(id, "%u\n", events[i]);
    if (write(out_fd, id, sizeof(id))) {
       fprintf(stderr,"Error writing to file descriptor");
      return 1;
    }
  }
  return 0;
}
