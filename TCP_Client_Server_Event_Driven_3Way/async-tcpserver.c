#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/fcntl.h>

#define MAX_CLIENTS 100
#define MAX_PENDING 20
#define MAX_LINE 20

//Define accept(s) function that accepts socket connection and puts new socket in file descriptor set
void handle_first_shake(int new_s);
void handle_second_shake(int new_s);

//handle first shake
void handle_first_shake(int new_s){
  char buf[MAX_LINE];
  bzero(buf, sizeof(buf));
  //Receive "HELLO #" from client
  
  
  recv(new_s, buf, sizeof(buf), 0);
  fputs(buf, stdout);
  fflush(stdout);
  fputc('\n', stdout);
  fflush(stdout);

  //Send "HELLO #+1"
  char buf1[MAX_LINE] = {0};
  int num;
  char hi[MAX_LINE];

  sscanf(buf, "%s %d", hi, &num);
  num++;
  sprintf(buf1, "HELLO %d", num);

  buf1[MAX_LINE-1] = '\0';
  int len = strlen(buf1)+1;

  send(new_s, buf1, len, 0);
}

//handle second shake
void handle_second_shake(int new_s){
  char buf[MAX_LINE];
  bzero(buf, sizeof(buf));
  recv(new_s, buf, sizeof(buf), 0);
  fputs(buf, stdout);
  fflush(stdout);
  fputc('\n', stdout);
  fflush(stdout);
}

struct clientState{
    int fd; //file descriptor
    int shake; //1 for first_handshake(), 2 for second_handshake()
  };

int main(int argc, char *argv[]) {
  char* host_addr = "127.0.0.1";
  int port = atoi(argv[1]);
  char buf[MAX_LINE];

  /*setup passive open*/
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) <0) {
    perror("simplex-talk: socket");
    exit(1);
  }

  /* Configure the server address */
  struct sockaddr_in sin;
  sin.sin_family = AF_INET; 
  sin.sin_addr.s_addr = inet_addr(host_addr);
  sin.sin_port = htons(port);
  // Set all bits of the padding field to 0
  memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));

  /* Bind the socket to the address */
  if((bind(s, (struct sockaddr*)&sin, sizeof(sin)))<0) {
    perror("simplex-talk: bind");
    exit(1);
  }

  // connections can be pending if many concurrent client requests
  listen(s, MAX_PENDING); 

  //create client state array to track each client
  struct clientState clientStateArray[MAX_CLIENTS]; 

  //Set fd_max = s
  int fdmax;
  fdmax = s;

  //Initialize and clear master and read sets
  fd_set master;
  fd_set read_fds;

  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  
  //Set 's' to non blocking using fcntl
  fcntl(s, F_SETFL, O_NONBLOCK);

  //Add 's' to master_set
  FD_SET(s, &master);
  
  //while loop
  while(1){

    //Initialize read_set using FD_ZERO
    FD_ZERO(&read_fds);
    //Copy master to read 
    read_fds = master;
    
    //use select on readfds
    if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
      perror("select");
      exit(4);
    }

    //For loop: cycle from i=0 to i=fd_max
    for(int i = 0; i <= fdmax; i++){

      //is there anything for reading?
      if(FD_ISSET(i, &read_fds)){
        if(i == s){
          int new_s;
          socklen_t len = sizeof(sin);
          
          //new_s = accept()....
          if((new_s = accept(s, (struct sockaddr *)&sin, &len)) <0){
            perror("simplex-talk: accept");
            exit(1);
          }

          //set new_s to non blocking
          fcntl(new_s, F_SETFL, O_NONBLOCK);

          //add new_s to master set
          FD_SET(new_s, &master);
          
          //add new_s to client array
          clientStateArray[new_s].fd = new_s;
          clientStateArray[new_s].shake = 1;

          if(new_s > fdmax){
            fdmax = new_s;
          }

        }else{
          for(int j=0; j <=fdmax; j++){
              //First handshake?
            if(FD_ISSET(j, &read_fds)){
              if(clientStateArray[j].shake == 1){
              
              handle_first_shake(j);
              clientStateArray[j].shake++;

              }else{ //second handshake
              
              handle_second_shake(j);
              close(clientStateArray[j].fd);
              FD_CLR(clientStateArray[j].fd, &master);
              clientStateArray[j].fd = 0;
              clientStateArray[j].shake = 0;
              }
            }
            
          }
          
        }
      }

    }

  }

  return 0;
}
