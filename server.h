#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <sys/signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/select.h>

#define QLEN            	10
#define MY_PORT_ID      	8000
#define MAXLINE         	512
#define END                     400
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

enum selection{
	SELECT,
	FORK,
	THREAD
};

int writen(int sd,char *ptr,int size);
int readn(int sd,char *ptr,int size);
void *doftp(void *);
void do_select(int );
void do_fork(int );
void do_thread(int );
int recvfile(int);
int sendfile(int);
int auth_user(int ssock);
int makedir(int ssock);
int listdir(int ssock);
int recursivedelete(char* ,int);
int deletefile(int);
void *do_multicast(void *);
int multicasts(char *);
void start_multicast(char *);
