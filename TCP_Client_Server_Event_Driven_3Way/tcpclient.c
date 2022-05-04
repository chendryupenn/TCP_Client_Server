#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_LINE 20

int main (int argc, char *argv[]) {
  char* host_addr = argv[1];
  int port = atoi(argv[2]);

  /* Open a socket */
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) <0){
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

  /* Connect to the server */
  if(connect(s, (struct sockaddr *)&sin, sizeof(sin))<0){
    perror("simplex-talk: connect");
    close(s);
    exit(1);
  }

  /*Send first Hello + argv[3]*/
  char buf[MAX_LINE] = {0};
 
  sprintf(buf, "HELLO %s", argv[3]);
  buf[MAX_LINE-1] = '\0';
  int len = strlen(buf)+1;

  send(s, buf, len, 0);
 

  /*Receive Hello argv[3] + 1 from server*/
 
  recv(s, buf, sizeof(buf), 0);
  fputs(buf, stdout);
  fflush(stdout);
  fputc('\n', stdout);
  
  /*Send Hello argv[3] + 2 to server and close*/
  char buf1[MAX_LINE] = {0};
  int num;
  char hi[MAX_LINE];
  
  sscanf(buf, "%s %d", hi, &num);

  //error check to make sure it's X+1
  if(num != (atoi(argv[3])+1)){
    printf("ERROR");
    fflush(stdout);
    fputc('\n', stdout);
    close(s);
  }

  num++;
  sprintf(buf1, "HELLO %d", num);
  
  buf1[MAX_LINE-1] = '\0';
  len = strlen(buf1)+1;

  send(s, buf1, len, 0);
  
  
    

  close(s);
  return 0;
}