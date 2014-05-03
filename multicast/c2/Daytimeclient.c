#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>

#define LINELEN 128
int main(int argc, char *argv[])
{
    int socketfd = 0,s = 0, n =0;
    char buffer[LINELEN + 1];
    char * msg ;
    struct sockaddr_in serveraddr;
    socklen_t * saddrlen;
//    memset(buffer,' ',sizeof(buffer));
    memset(&serveraddr, '0', sizeof(serveraddr));

    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    if(socketfd < 0)
    {
        printf("Error in socket creation\n");
        exit(-1);
    }
    else
    {
        printf("Client : Socket creation successful\n");
    }
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = 13;
  //  serveraddr.sin_addr.s_addr = gethostbyname("localhost");
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    s = connect(socketfd, (struct sockaddr *)&serveraddr,sizeof(serveraddr)); 
    if(s < 0){
        printf("Error connecting to server\n");
        exit(-1);
    }
    else
    {
        printf("Client : Connected to server\n");
    }
   
    n = read(s, buffer, LINELEN);
    if(n > 0)
    {
        buffer[n] = '\0';
        (void) fputs(buffer, stdout);
    }
    else
    {
        printf("error reading\n");
    }
    
    close(socketfd);    
    return 0;
}
