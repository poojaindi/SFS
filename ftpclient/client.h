#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#define STR 200
#define RTR 300
#define END 400
#define MAXLINE                 512
#define ACK                     2
#define NACK                    3
#define REQUESTFILE             100
#define STOREFILE               200
#define MKDIR                   300
#define LIST                    500
#define DELETEFILE              600
#define COMMANDNOTSUPPORTED     150
#define COMMANDSUPPORTED        160
#define BADFILENAME             200
#define FILENAMEOK              400
#define STARTTRANSFER           500
#define FAILURE                 350
#define SUCCESS                 250
#define NO_USER                 260
#define INVALID_PASS            270
#define LOGIN_SUCCESS           280

int errexit(const char *format, ...);
int connectTCP(const char *hostname, const char *service);
int TCPecho(const char *hostname, const char *service);
int readn(int sd,char *ptr,int size);
int writen(int sd,char *ptr,int size);
void readfile(int,char *);
void storefile(int,char *);
void makedir(int connectfd, char *path);
void listdir(int connectfd, char *path);
void auth_user (int connectfd, char *user, char *pass);
void deletefile(int , char * ,char * );
