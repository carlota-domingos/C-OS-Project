#include "api.h"


int session_id;
char this_req_pipe_path[40];
char this_resp_pipe_path[40];
int fres, freq;

int ems_setup(char const* req_pipe_path, char const* resp_pipe_path, char const* server_pipe_path) {
  int fdserver;
  char code = '1';
  char buffer[40];

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
  
  strcpy(buffer, req_pipe_path);
  for(long unsigned int i =strlen(req_pipe_path); i<40; i++){
    buffer[i] = '\0';
  }
  printf("%s\n", buffer);
  if((fdserver = open(server_pipe_path, O_WRONLY))==-1){
    fprintf(stderr, "Failed to open server pipe\n");
    return 1;
  } 
  if (write(fdserver, &code, 1) < 0){
    fprintf(stderr, "Failed to write to server pipe\n");
    return 1;
  }
  if (write(fdserver, &buffer, 40) < 0){
    fprintf(stderr, "Failed to write to server pipe\n");
    return 1;
  }
  strcpy(buffer, resp_pipe_path);
  for(long unsigned int i =strlen(resp_pipe_path); i<40; i++){
    buffer[i] = '\0';
  }
  printf("%s\n", buffer);
  
  if (write(fdserver, &buffer, 40) < 0){
    fprintf(stderr, "Failed to write to server pipe\n");
    return 1;
  }
  printf("before closing\n");
  if (close(fdserver)<0){
    fprintf(stderr, "Failed to close server pipe\n");
    return 1;
  }
  printf("after closing\n");

  if ((freq = open (req_pipe_path, O_WRONLY)) < 0) {
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
  
  if ((fres = open (resp_pipe_path, O_RDONLY)) < 0){
    if(unlink(req_pipe_path)<0){
      fprintf(stderr, "Failed to unlink request pipe\n");
    }
    if(unlink(resp_pipe_path)<0){
      fprintf(stderr, "Failed to unlink reponse pipe\n");
    }
    fprintf(stderr, "Failed to open response pipe\n");
    return 1;
  }

  
  if(read(fres, &session_id, sizeof(int))<0){
    fprintf(stderr, "Failed to read from server pipe\n");
    return 1;
  } 
  
  printf("session_id: %d\n", session_id);
  strcpy(this_req_pipe_path, req_pipe_path);
  printf("this_req_pipe_path: %s\n", this_req_pipe_path);
  strcpy(this_resp_pipe_path, resp_pipe_path);
  printf("this_resp_pipe_path: %s\n", this_resp_pipe_path);

  return 0;
}

int ems_quit(void) { 
  char code = '2';
  if (write(freq, &code, 1) < 0){
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
  char code ='3';

  if(write(freq, &code,1)<0) {
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

  if(read(fres, &ret, sizeof(int))<0) {
    fprintf(stderr, "Failed to read response from response pipe\n");
    return 1;
  }
  return ret;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  int ret;
  char code = '4';
  if(write(freq,&code,1)<0) {
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
  size_t num_rows, num_cols;
  int ret;
  char code= '5'; 
  if (write(freq,&code,1) < 0) {
    fprintf(stderr, "Failed to write command to request pipe\n");
    return 1;
  }

  if (write(freq,&event_id,sizeof(unsigned int)) < 0) {
    fprintf(stderr, "Failed to write event_id to request pipe\n");
    return 1;
  }

  if(read(fres, &ret, sizeof(int))<0) {
    fprintf(stderr, "Failed to read response from response pipe\n");
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
  unsigned int *seats= malloc(sizeof(unsigned int)*num_rows*num_cols);
  if(read(fres, seats, sizeof(unsigned int)*num_rows*num_cols)<0) {
    fprintf(stderr, "Failed to read seats array from response pipe\n");
    return 1;
  }

  //buffer para collecionar tudo CUIDADO se for pequeno aumentar tamanho
  char showStr[1028]= "";
  size_t showStrCounter;
  int temp;
  for (size_t i = 0; i <num_rows; i++) {
    for (size_t j = 0; j <num_cols; j++) {
      char buffer[16];
      temp = sprintf(buffer, "%u", seats[i*num_cols+j]);
      showStrCounter += (size_t)temp;
      strcat(showStr,buffer);

      if (j <num_cols) {
        strcat(showStr, " ");
        showStrCounter++;
      }
    }

    strcat(showStr, "\n");
    showStrCounter++;

  }
  strcat(showStr, "\0");
  showStrCounter++;
  //lock_fd
  if (write(out_fd, showStr ,showStrCounter-1)==-1) {
    perror("Error writing to file");
    free(seats);
    return 1;
  }
  //unlock_fd
  free(seats);
  return 0;

}
  

int ems_list_events(int out_fd) {
  int ret;
  char code = '6';
  size_t num_events;
  if (write(freq,&code,1) < 0) {
    fprintf(stderr, "Failed to write command to request pipe\n");
    return 1;
  }

  if(read(fres, &ret, sizeof(int))<0) {
    fprintf(stderr, "Failed to read return\n");
  } else if (ret == 1) {
    fprintf(stderr, "Failed to list events\n");
    return 1;
  }

  if(read(fres, &num_events, sizeof(size_t))<0) {
    fprintf(stderr, "Failed to read num_events\n");
    return 1;
  }
  if(num_events == 0){
    char buffer[] = "No events \n\0";
    if (write(out_fd, buffer,sizeof(buffer))==-1) {
      fprintf(stderr,"Error writing to file descriptor");
      return 1;
    }
    return 0;
  }
    
  unsigned int events[num_events];
  if(read(fres, &events , sizeof(unsigned int)*num_events)<0){
    fprintf(stderr, "Failed to read events array\n");
    return 1;
  }
  //buffer para collecionar tudo CUIDADO se for pequeno aumentar tamanho
  char listStr[1028]= "";
  size_t listStrCounter=0;
  int temp;
  for (size_t i = 0; i <num_events; i++) {
    strcat(listStr, "Event: ");
    listStrCounter += 7;

    char id[16];
    temp = sprintf(id, "%u\n", events[i]);
    strcat(listStr, id);
    listStrCounter += (size_t)temp;
  }

  strcat(listStr, "\0");
  listStrCounter++;

  if(write(out_fd, listStr, listStrCounter-1) == -1) {
    perror("Error writing to file");
    return 1;
  }
  return 0;
}
