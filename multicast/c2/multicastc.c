#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
 
#define MAXSIZE 512;

int main (int argc, char *argv[]){ 
	struct sockaddr_in    localSock;
	socklen_t             laddrlen;
	struct ip_mreq        group;
	int                   sd;
	int                   datalen, i;
	char                  databuf[1024];
	char                  filename[50] = "";
	char                  file_buffer[512];
	FILE *                fp;
 	int                   num_blks, num_blks1;
	int                   num_last_blks, num_last_blks1;
  /* ------------------------------------------------------------*/
  /*                                                             */
  /* Receive Multicast Datagram code example.                    */
  /*                                                             */
  /* ------------------------------------------------------------*/
 
  /*
   * Create a datagram socket on which to receive.
   */
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0) {
    perror("opening datagram socket");
    exit(1);
  }
 
  /*
   * Enable SO_REUSEADDR to allow multiple instances of this
   * application to receive copies of the multicast datagrams.
   */
    int reuse=1;
 
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&reuse, sizeof(reuse)) < 0) {
      perror("setting SO_REUSEADDR");
      close(sd);
      exit(1);
    }
 
  /*
   * Bind to the proper port number with the IP address
   * specified as INADDR_ANY.
   */
  memset((char *) &localSock, 0, sizeof(localSock));
  localSock.sin_family = AF_INET;
  localSock.sin_port = htons(5555);;
  localSock.sin_addr.s_addr  = INADDR_ANY;
  laddrlen = sizeof(localSock);
 
  if (bind(sd, (struct sockaddr*)&localSock, sizeof(localSock))) {
    perror("binding datagram socket");
    close(sd);
    exit(1);
  }
 
 
  /*
   * Join the multicast group 225.1.1.1 on the local 9.5.1.1
   * interface.  Note that this IP_ADD_MEMBERSHIP option must be
   * called for each local interface over which the multicast
   * datagrams are to be received.
   */
  group.imr_multiaddr.s_addr = inet_addr("225.1.1.1");
  group.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 (char *)&group, sizeof(group)) < 0) {
    perror("adding multicast group");
    close(sd);
    exit(1);
  }
 
  /*
   * Read from the socket.
   */

  while(1){
/*
  datalen = sizeof(databuf);
  if (recvfrom(sd, databuf, datalen,0, (struct sockaddr*) &localSock,
			  &laddrlen) < 0) {
    perror("reading datagram message");
    close(sd);
    exit(1);
  }
*/

  //printf("%s\n",databuf); 
/*  printf("Enter filename\n");
  scanf("%s", filename);
  if(sendto(sd, filename, strlen(filename), 0, (struct sockaddr*) &localSock,
			  laddrlen) < 0){
	  perror("writing filename");
	  close(sd);
	  exit(1);
  }*/
	printf("\nBackup server is waiting for multicast data...............\n");
        printf(".........................................................\n");
  if(recvfrom(sd, filename, sizeof(filename), 0, (struct sockaddr*)
			  &localSock, &laddrlen) < 0){
	  perror("reading filename");
	  close(sd);
	  exit(1);
  }
	printf("File name received: %s\n",filename);

  if((fp = fopen(filename, "w")) < 0)
  {
	  perror("Opening in write mode");
	  close(sd);
	  exit(1);
  }
  
  recvfrom(sd, &num_blks, sizeof(num_blks), 0,  (struct sockaddr*)
		  	&localSock, &laddrlen);
  num_blks1 = ntohs(num_blks);
//  printf("Got the no. of blocks %d\n",num_blks);
  recvfrom(sd, &num_last_blks, sizeof(num_last_blks), 0,  (struct sockaddr*)
		  	&localSock, &laddrlen);
  num_last_blks1 = ntohs(num_last_blks);
 // printf("Got the last remaining byte size %d\n", num_last_blks1);

  for(i= 0; i < num_blks1; i ++) {
	
	recvfrom(sd, file_buffer, sizeof(file_buffer), 0, (struct sockaddr*)
			&localSock, &laddrlen);

  	if((fwrite(file_buffer,sizeof(char),512,fp)) < 0){
		perror("Writing blocks");
		close(sd);
		exit(1);
	}
  }

  if(num_last_blks1 > 0){
	  recvfrom(sd, file_buffer, num_last_blks1, 0, (struct sockaddr*)
			  &localSock, &laddrlen);
	  fwrite(file_buffer, sizeof(char), num_last_blks1,fp);
  }
  fclose(fp);
  printf("A copy of file: %s has been created in this server!!!\n",filename);
  num_blks = 0;
  num_blks1 = 0;
  num_last_blks = 0;
  num_last_blks1 = 0;
  memset(file_buffer, 0, sizeof(file_buffer));
  memset(filename, 0, sizeof(filename));
  memset(databuf, 0, sizeof(databuf));
  
  }
}
