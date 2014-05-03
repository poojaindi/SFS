/* Server program of a file transfer service. This is a "concurrent     */
/* server" that can handle requests from multiple simultaneous clients. */

#include "server.h"

static int option;
int status;

/*End execution in different ways for select, fork and thread*/
int endexecution(int opt)
{
	switch(opt)
	{
		case 333:
			return -1; 
		case 444:
			exit(0);
			break;
		case 555:
			return ;
		default:;
	}
}
 
struct db_entry
{
	char *username;          // username
	char *password;          // password
	char *home_dir;          // home directory
	unsigned short user_id;  // user ID
	unsigned short group_id; // group ID
	struct db_entry *next;   // pointer to next user
};

int main()  
{
	int msock, sel;
	char *service = "ftp";
	enum selection option;

	printf("Please enter number to select corresponding option:\n");
	printf("0: select\n1:fork\n2:thread\n");
	scanf("%d", &sel);
	option = sel;

	msock = passiveTCP(service, QLEN);

	switch(option)
	{
		case SELECT: 
			printf("SELECT option was selected\n");
			do_select(msock);
			break;
		case FORK:
			printf("FORK option was selected\n");
			do_fork(msock);
			break;
		case THREAD:
			printf("THREAD option was selected\n");
			do_thread(msock);
			break;
		default:	
			printf("Invalid option\n");
			exit(1);
	}

}   

int passiveTCP(char *service, int qlen)
{
	struct sockaddr_in server;
	struct servent *serv;
	int sockfd, optval = 1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
	{
		printf("server: error setting SO_REUSEADDR socket option: %s\n", strerror(errno));
		exit(1);
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;

	if(serv = getservbyname(service, "tcp"))
		server.sin_port = serv->s_port;
	else
	{
		printf("server: getservbyname couldn't find %s\n", service);
		exit(1);
	}

	if(bind(sockfd, (struct sockaddr*)&server, sizeof(server)) <0)
	{
		printf("server: bind error: %s\n", strerror(errno));
		exit(1);
	}

	listen(sockfd,qlen);
	return sockfd;
}

void do_select(int msock)
{
	int max_fd, msg_ok, ret = 0, req, max_index, i, client_fd, c_len, total_ready;
	fd_set rset, wholeset;
	int client[FD_SETSIZE], n, sock_fd;
	char buffer[MAXLINE];
	struct sockaddr_in client_addr;

	max_fd = msock;
	max_index = -1;

	for(i=0; i<FD_SETSIZE; i++)
		client[i] = -1;

	FD_ZERO(&wholeset);
	FD_SET(msock, &wholeset);

	while (1)
	{
		memcpy(&rset, &wholeset, sizeof(rset));
		total_ready = select(max_fd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(msock, &rset))
		{
			printf("Waiting for client connection\n");
			//accept an incoming connection
			c_len = sizeof(struct sockaddr_in);
			client_fd = accept(msock, (struct sockaddr *)&client_addr, 
					(socklen_t *)&c_len);
			if(client_fd == -1)
			{
				perror("ERROR: accept()\n");
				exit(1);
			}

			ret = auth_user(client_fd);
			if (ret == -1)
				close(client_fd);
			else
			{
				for(i=0; i<FD_SETSIZE; i++)
				{
					if (client[i] < 0)
					{
						client[i] = client_fd;
						break;
					}
				}

				if (i==FD_SETSIZE)
				{
					perror("Too many clients\n");
					exit(1);
				}
				FD_SET(client_fd, &wholeset);

				if (client_fd > max_fd)
					max_fd = client_fd;

				if(i > max_index)
					max_index = i;

				if (--total_ready <= 0)
					continue;
			}
		}

		for(i=0; i<=max_index; i++)
		{
			if ((sock_fd = client[i]) < 0)
				continue;

			if(FD_ISSET(sock_fd, &rset))
			{
				//doftp((void *)(long)sock_fd);
				n = readn(sock_fd,(char *)&req,sizeof(req));

				if(n <= 0)
				{
					close(sock_fd);
					FD_CLR(sock_fd, &wholeset);
					client[i] = -1;

					if (n < 0)
						printf("server: read error %d\n",errno);
					//status=endexecution(option);
				}
				else
				{
					req = ntohs(req);
					switch(req)
					{
					case STOREFILE:
						status=sendfile(sock_fd);
						break;
					case REQUESTFILE:
						status=recvfile(sock_fd);
						break;
					case MKDIR:
						status=makedir(sock_fd);
						break;
					case LIST:
						status=listdir(sock_fd);
						break;
					case DELETEFILE:
						status=deletefile(sock_fd);
						break;
					case END:
						printf("Client sent request to end connection.\n");
						close(sock_fd);
						FD_CLR(sock_fd, &wholeset);
						client[i] = -1;
						break;
					default:
						break;
					}
				}
				if(--total_ready <= 0)
					break;
			}
		}
	}
}

void do_fork(int msock)
{
	int ssock, clilen;
	struct sockaddr_in client_addr;   

	while(1)
	{
		printf("Waiting for client connection\n");
		ssock = accept(msock ,(struct sockaddr *) &client_addr, &clilen);
		if (ssock < 0)
		{
			printf("server: accept error: %s\n", strerror(errno)); 
			exit(1); 
		}

		switch(fork())
		{
			case 0:
				close(msock);
				doftp((void *)(long)ssock);
				exit(1);
				break;
			default:
				close(ssock);
				break;
			case -1:
				printf("Failed to fork\n");
				exit(1);
		}
	}
}

void do_thread(int msock)
{
	int ssock, clilen;
	struct sockaddr_in client_addr;   
	pthread_t th;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	while(1)
	{
		printf("Waiting for client connection\n");
		ssock = accept(msock ,(struct sockaddr *) &client_addr, &clilen);
		if (ssock < 0)
		{
			printf("server: accept error: %s\n", strerror(errno)); 
			exit(1); 
		}

		if (pthread_create(&th, &attr, doftp, (void *)(long)ssock))
		{	
			printf("server: pthread_create error: %s\n", strerror(errno));
			exit(1);
		}
	}
}	

void *doftp(void *sd)
{
	int req, msg_ok, ret = 0,dump;
	long ssock = (long)sd;

	ret = auth_user(ssock);
	if (ret == -1)
	{
		close(ssock);
		return;
	}

	while(1)
	{
		req = 0;

		if((readn(ssock,(char *)&req,sizeof(req))) < 0)
		{
			printf("server: read error %d\n",errno);
			return;
		}

		req = ntohs(req);
		switch(req)
		{
			case STOREFILE:
				status=sendfile(ssock);
				break;
			case REQUESTFILE:
				status=recvfile(ssock);
				break;
			case MKDIR:
				status=makedir(ssock);
				break;
			case LIST:
				status=listdir(ssock);
				break;
			case DELETEFILE:
				status=deletefile(ssock);
				break;
			case END:
				printf("Client sent request to end connection.\n");
				close(ssock);
				return;
			default:
				break;
		}
		if(status==-1)
			break;
	}
	return;
}

void start_multicast(char * filename)
{
	pthread_t multicast;
	pthread_create(&multicast,NULL,do_multicast,(void *)filename);
	pthread_join(multicast,NULL);
	return;
}

void * do_multicast(void *p)
{
	int localstat;
	printf("multicast statrted\n");
	char * filename= (char *)p;

	printf("file name %s \n",filename);
	multicasts(filename);

	return;

}

int deletefile(int ssock)
{
	int  getfile,ack,msg,msg_2,c,len,n,msg_ok,fail,temp,req;
	char fname[MAXLINE] = "";
	FILE *fp;
	DIR *dp;

	char abs_filename[MAXLINE];	
	struct stat fileinfo;
	n = read(ssock,fname,sizeof(fname));
	fname[n]='\0';
	if(n < 0){
		printf("server: filename read error :%d\n",errno);
		fail = BADFILENAME ;
	}
	else
		printf("File name to be deleted: %s\n",fname);
	if (lstat(fname, &fileinfo) < 0)
	{  
		perror ( fname );	
		temp=FAILURE;
		if((writen(ssock,(char *)&temp,sizeof(temp))) < 0)
		{
			printf("server: write error :%d\n",errno);
			return(endexecution(option));
		}
		return(endexecution(option));
	}
	if(S_ISDIR(fileinfo.st_mode)) 
	{
		printf("Its a directory:\n");
		recursivedelete(fname,ssock) ;
		return(endexecution(option));
	}
	if(remove(fname)<0)
		printf("server: error opening file: %s\n", strerror(errno));
	else 	
		printf("File deleted successfully\n");
	temp=SUCCESS;
	if((writen(ssock,(char *)&temp,sizeof(temp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));
	}

	return(endexecution(option));

}//End of deletefle

int recursivedelete(char* dirname,int ssock) 
{

	struct dirent *pdir;
	int temp;
	DIR *dir;
	char filename[FILENAME_MAX];

	dir = opendir (dirname);
	if (dir != NULL)
	{
		while (pdir = readdir (dir)) {
			struct stat stFileInfo;

			snprintf(filename, FILENAME_MAX, "%s/%s", dirname, pdir->d_name);

			if (lstat(filename, &stFileInfo) < 0)
				perror ( filename );

			if(S_ISDIR(stFileInfo.st_mode)) {
				if(strcmp(pdir->d_name, ".") && 
						strcmp(pdir->d_name, "..")) 
				{
					printf("%s directory\n",filename);

					recursivedelete(filename,ssock);
				}
			} else {
				printf("%s file\n",filename);
				remove(filename);
			}
		}
		(void) closedir (dir);
	}
	else
	{
		perror ("The directory can't be opened");
		temp=FAILURE;
		if((writen(ssock,(char *)&temp,sizeof(temp))) < 0)
		{
			printf("server: write error :%d\n",errno);
			return(endexecution(option));
		}
		return -1;
	}
	remove(dirname);
	printf("Directory: %s has been deleted successfully\n",dirname);
	temp=SUCCESS;
	if((writen(ssock,(char *)&temp,sizeof(temp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));
	}
	return 1;

}


int findUser(const char *user, const char *pass, struct db_entry *p_entry)
{
	FILE *fp = fopen("db.txt", "rb");
	char line[MAXLINE];
	char *pchar;

	if (fp == NULL)
	{
		printf("server: error opening file: %s\n", strerror(errno));
		//return(endexecution(option));
	}

	memset(line, 0, MAXLINE);

	while(fgets(line, MAXLINE, fp) != NULL)
	{
		pchar = strtok(line, ":\n");

		if (!strcmp(pchar, user))
		{
			p_entry->username = (char *)malloc(sizeof(char) * MAXLINE);
			strcpy(p_entry->username, pchar);

			pchar = strtok(NULL, ":\n");
			if (!strcmp(pchar, pass))
			{
				p_entry->password = (char *)malloc(sizeof(char) * MAXLINE);
				strcpy(p_entry->password, pchar);

				p_entry->home_dir = (char *)malloc(sizeof(char) * MAXLINE);
				strcpy(p_entry->home_dir, strtok(NULL, ":\n"));

				pchar = strtok(NULL, ":\n");
				if (pchar)
					p_entry->user_id = (unsigned short)(*pchar);

				pchar = strtok(NULL, ":\n");
				if (pchar)
					p_entry->group_id = (unsigned short)(*pchar);

				return LOGIN_SUCCESS;
			}
			else
				return INVALID_PASS;
		}
	}
	return NO_USER;
}
			
int auth_user(int ssock)
{
	char uname[MAXLINE] = "";
	char pwd[MAXLINE] = "";
	int fail, tmp, ret;
	struct db_entry p_entry;

	if((read(ssock,uname, MAXLINE)) < 0) 
	{
		printf("server: username read error :%d\n",errno);
		//return(endexecution(option));
	}

	if (!strcmp(uname, ""))
		return -1;

	printf("server: username received is %s\n",uname);
	tmp = htons(SUCCESS);
	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		//return(endexecution(option));   
	}

	if((read(ssock,pwd, MAXLINE)) < 0) 
	{
		printf("server: pwd read error :%d\n",errno);
		//return(endexecution(option));
	}
	if (!strcmp(pwd, ""))
		return -1;

	printf("server: pwd is %s\n",pwd);

	//crypted = crypt(pwd, salt);

	tmp = htons(SUCCESS);
	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		//return(endexecution(option));  
	}

	//inputF = fopen("db.txt", "rb");

	// check opening correctness
	//if(inputF == NULL) 
	//       fprintf(stderr, "error during opening filename db.txt\n");

	//p_entry = (struct db_entry *)malloc(sizeof(struct db_entry));
	ret = findUser(uname, pwd, &p_entry);
	tmp = htons(ret);

	if (ret == NO_USER)
	{
		printf("server: Invalid username. Closing client connection.\n");	
		if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
		{
			printf("server: write error :%d\n",errno);
			close(ssock);
			//return(endexecution(option));
		}
		return -1;
	}
	else if (ret == INVALID_PASS)
	{
		printf("server: Invalid password. Closing client connection.\n");
		if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
		{
			printf("server: write error :%d\n",errno);
			//return(endexecution(option));
		}
		return -1;
	}
	else
	{
		printf("server: Login successful.\n");
		if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
		{
			printf("server: write error :%d\n",errno);
			//return(endexecution(option));
		}
	}
}

int listdir(int ssock)
{
	char path[MAXLINE] = "", buf[MAXLINE] = "";
	int tmp, msg, n;
	DIR *dp;
	struct dirent *ep;
    char * endmsg = "END";
	msg = COMMANDSUPPORTED; 
	msg = htons(msg);
	if((writen(ssock,(char *)&msg,sizeof(msg))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));
	}
	if((read(ssock,path, MAXLINE)) < 0) 
	{
		printf("server: pathname read error :%d\n",errno);
		return(endexecution(option));
	}

	printf("server: pathname received is %s\n", path);

	dp = opendir(path);

	if (dp != NULL)
	{
		printf("Files in the directory are:\n");
		while(ep = readdir(dp))
		{
			printf("%s\n", ep->d_name);
			if((n = write(ssock, ep->d_name, strlen(ep->d_name))) < 0)
				printf("server: write error :%d\n",errno);
//			printf("no of bytes written : %d\n",n);
			n = read(ssock, path, strlen(path));
		}
		msg = htons(SUCCESS);
		if((write(ssock, endmsg, sizeof(endmsg))) < 0)
			printf("server: write error :%d\n",errno);
		if((write(ssock, (char *)&msg, sizeof(msg))) < 0)
			printf("Server: write error :%d\n",errno);
		(void) closedir(dp);
	}
	else
	{
		//printf ("Couldn't open the directory");
		strcpy(buf, "Couldn't open directory ");
		strcat(buf, path);
		printf("server: %s\n", buf);

		if((writen(ssock, buf, strlen(buf))) < 0)
			printf("server: write error :%d\n",errno);
	}
}

int makedir(int ssock)
{
	char path[MAXLINE] = "";
	int tmp, msg;

	msg = COMMANDSUPPORTED; 
	msg = htons(msg);
	if((writen(ssock,(char *)&msg,sizeof(msg))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));
	}

	if((read(ssock,path, MAXLINE)) < 0) 
	{
		printf("server: pathname read error :%d\n",errno);
		return(endexecution(option));
	}

	printf("server: pathname received is %s\n", path);

	tmp = mkdir(path, S_IRWXU);
	if (tmp == 0)
		printf("server: directory created successfully\n");
	else
		printf("server: error while creating directory: %s\n", strerror(errno));

	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return;//(endexecution(option));   
	}
}
	
int sendfile(int ssock)
{
	int  newsockid,i,getfile,ack,msg,msg_2,c,len,n,msg_ok,fail,tmp,req;
	int no_writen,start_xfer, num_blks,num_last_blk;
	struct sockaddr_in my_addr, server_addr; 
	char fname[MAXLINE] = "";
	FILE *fp; 
	char in_buf[MAXLINE];
	no_writen = 0;
	num_blks = 0;
	num_last_blk = 0;
	msg_ok = COMMANDSUPPORTED; 
	msg_ok = htons(msg_ok);
	if((writen(ssock,(char *)&msg_ok,sizeof(msg_ok))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));
	}
	///fflush(fname);
	fail = FILENAMEOK;
	n = read(ssock,fname,sizeof(fname));
	fname[n]='\0';
	if(n < 0)
	{
		printf("server: filename read error :%d\n",errno);
		fail = BADFILENAME ;
	}
	if((fp = fopen(fname,"w")) == NULL)/*cant open file*/
		fail = BADFILENAME;

	tmp = htons(fail);
	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));   
	}
	if(fail == BADFILENAME)
	{
		printf("server cant open file\n");
		return;
	}
	printf("server: filename is %s\n",fname);
	req = 0;
	if ((readn(ssock,(char *)&req,sizeof(req))) < 0)
	{
		printf("server: read error :%d\n",errno);
		return(endexecution(option));
	}
//	printf("server: start transfer command, %d, received\n", ntohs(req));

	if((readn(ssock,(char *)&num_blks,sizeof(num_blks))) < 0)
	{
		printf("server read error on nblocks :%d\n",errno);
		return(endexecution(option));
	}
	num_blks = ntohs(num_blks);
	printf("server: client sent: %d blocks in file\n",num_blks);
	ack = ACK;  
	ack = htons(ack);
	if((writen(ssock,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("server: ack write error :%d\n",errno);
		return(endexecution(option));
	}
	if((readn(ssock,(char *)&num_last_blk,sizeof(num_last_blk))) < 0)
	{
		printf("server: read error :%d on nbytes\n",errno);
		return(endexecution(option));
	}
	num_last_blk = ntohs(num_last_blk);  
	printf("server: client responded: %d bytes last blk\n",num_last_blk);
	if((writen(ssock,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("server: ack write error :%d\n",errno);
		return(endexecution(option));
	}


	/* BEGIN READING BLOCKS BEING SENT BY CLIENT */
	printf("server: starting to get file contents\n");
	for(i= 0; i < num_blks; i ++) 
	{
		if((readn(ssock,in_buf,MAXLINE)) < 0)
		{
			printf("server: block error read: %d\n",errno);
			return(endexecution(option));
		}
		no_writen = fwrite(in_buf,sizeof(char),MAXLINE,fp);
		if (no_writen == 0) 
		{
			printf("server: file write error\n");
			return(endexecution(option));
		}
		if (no_writen != MAXLINE) 
		{
			printf("server: file write  error : no_writen is less\n");
			return(endexecution(option));
		}
		/* send an ACK for this block */
		if((writen(ssock,(char *)&ack,sizeof(ack))) < 0)
		{
			printf("server: ack write  error :%d\n",errno);
			return(endexecution(option));
		}
		printf(" %d...",i);
	}


	/*IF THERE IS A LAST PARTIALLY FILLED BLOCK, READ IT */

	if (num_last_blk > 0) 
	{
		printf("%d\n",num_blks);      
		if((readn(ssock,in_buf,num_last_blk)) < 0)
		{
			printf("server: last block error read :%d\n",errno);
			return(endexecution(option));
		}
		no_writen = fwrite(in_buf,sizeof(char),num_last_blk,fp); 
		if (no_writen == 0) 
		{
			printf("server: last block file write err :%d\n",errno);
			return(endexecution(option));
		}
		if (no_writen != num_last_blk) 
		{
			printf("server: file write error : no_writen is less 2\n");
			return(endexecution(option));
		}
		if((writen(ssock,(char *)&ack,sizeof(ack))) < 0)
		{
			printf("server:ack write  error  :%d\n",errno);
			return(endexecution(option));
		}
	}
	else 
		printf("\n");


	/*FILE TRANSFER ENDS. CLIENT TERMINATES AFTER  CLOSING ALL ITS FILES
	  AND SOCKETS*/ 

	fclose(fp);
       	start_multicast(fname);
	printf("server: FILE TRANSFER COMPLETE\n");
	//close(ssock);

}
     

int  recvfile(int ssock)
{       
	//	int ssock=*((int *)sd);
	int i,fsize,fd,msg_ok,fail,fail1,req,c,ack,n;
	int no_read ,num_blks , num_blks1,num_last_blk,num_last_blk1,tmp;
	char fname[MAXLINE] = "";
	char out_buf[MAXLINE];
	FILE *fp;

	no_read = 0;
	num_blks = 0;
	num_last_blk = 0; 


	/* START SERVICING THE CLIENT */ 

	/* get command code from client.*/
	/* only one supported command: 100 -  get a file */
	/* reply to client: command OK  (code: 160) */
	msg_ok = COMMANDSUPPORTED; 
	msg_ok = htons(msg_ok);
	if((writen(ssock,(char *)&msg_ok,sizeof(msg_ok))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));
	}
	fail = FILENAMEOK;
	n = read(ssock,fname,sizeof(fname));
	if(n < 0)
	{
		printf("server: filename read error :%d\n",errno);
		fail = BADFILENAME ;
	}

	/* IF SERVER CANT OPEN FILE THEN INFORM CLIENT OF THIS AND TERMINATE */
	if((fp = fopen(fname,"r")) == NULL)/*cant open file*/
		fail = BADFILENAME;

	tmp = htons(fail);
	if((writen(ssock,(char *)&tmp,sizeof(tmp))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));
	}
	if(fail == BADFILENAME)
	{
		printf("server cant open file\n");
		return(endexecution(option));
	}
	printf("server: filename is %s\n",fname);
	req = 0;
	if ((readn(ssock,(char *)&req,sizeof(req))) < 0)
	{
		printf("server: read error :%d\n",errno);
		return(endexecution(option));
		//return NULL;
	}
//	printf("server: start transfer command, %d, received\n", ntohs(req));
	/*SERVER GETS FILESIZE AND CALCULATES THE NUMBER OF BLOCKS OF 
	  SIZE = MAXLINE IT WILL TAKE TO TRANSFER THE FILE. ALSO CALCULATE
	  NUMBER OF BYTES IN THE LAST PARTIALLY FILLED BLOCK IF ANY. 
	  SEND THIS INFO TO CLIENT, RECEIVING ACKS */
	printf("server: starting transfer\n");
	fsize = 0;ack = 0;   
	while ((c = getc(fp)) != EOF) {fsize++;}
	num_blks = fsize / MAXLINE; 
	num_blks1 = htons(num_blks);
	num_last_blk = fsize % MAXLINE; 
	num_last_blk1 = htons(num_last_blk);
	if((writen(ssock,(char *)&num_blks1,sizeof(num_blks1))) < 0)
	{
		printf("server: write error :%d\n",errno);
		//return NULL;
		return(endexecution(option));
	}
	printf("server: told client there are %d blocks\n", num_blks);  
	if((readn(ssock,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("server: ack read error :%d\n",errno);
		return(endexecution(option));
		//return NULL;
	}          
	if (ntohs(ack) != ACK) 
	{
		printf("client: ACK not received on file size\n");
		return(endexecution(option));
		//return NULL;
	}
	if((writen(ssock,(char *)&num_last_blk1,sizeof(num_last_blk1))) < 0)
	{
		printf("server: write error :%d\n",errno);
		return(endexecution(option));//return NULL;
	}
	printf("server: told client %d bytes in last block\n", num_last_blk);  
	if((readn(ssock,(char *)&ack,sizeof(ack))) < 0)
	{
		printf("server: ack read error :%d\n",errno);
		return(endexecution(option));//return NULL;
	}
	if (ntohs(ack) != ACK)
	{
		printf("server: ACK not received on file size\n");
		return(endexecution(option));
		// return NULL;
	}
	rewind(fp);    
	/* ACTUAL FILE TRANSFER STARTS  BLOCK BY BLOCK*/       
	for(i= 0; i < num_blks; i ++) 
	{ 
		no_read = fread(out_buf,sizeof(char),MAXLINE,fp);
		if (no_read == 0) 
		{
			printf("server: file read error\n");
			return(endexecution(option));
		}
		if (no_read != MAXLINE)
		{
			printf("server: file read error : no_read is less\n");
			return(endexecution(option));
		}
		if((writen(ssock,out_buf,MAXLINE)) < 0)
		{
			printf("server: error sending block:%d\n",errno);
			return(endexecution(option));
		}
		if((readn(ssock,(char *)&ack,sizeof(ack))) < 0)
		{
			printf("server: ack read  error :%d\n",errno);
			return(endexecution(option));
		}
		if (ntohs(ack) != ACK) 
		{
			printf("server: ACK not received for block %d\n",i);
			return(endexecution(option));
			//return NULL;
		}
		printf(" %d...",i);
	}

	if (num_last_blk > 0) 
	{ 
		printf("%d\n",num_blks);
		no_read = fread(out_buf,sizeof(char),num_last_blk,fp); 
		if (no_read == 0) 
		{
			printf("server: file read error\n");
			return(endexecution(option));
		}
		if (no_read != num_last_blk) 
		{
			printf("server: file read error : no_read is less 2\n");
			return(endexecution(option));
		}
		if((writen(ssock,out_buf,num_last_blk)) < 0)
		{
			printf("server: file transfer error %d\n",errno);
			return(endexecution(option));
		}
		if((readn(ssock,(char *)&ack,sizeof(ack))) < 0)
		{
			printf("server: ack read  error %d\n",errno);
			return(endexecution(option));
		}
		if (ntohs(ack) != ACK) 
		{
			printf("server: ACK not received last block\n");
			return(endexecution(option));
			// return NULL;
		}
	}
	else 
		printf("\n");

	/* FILE TRANSFER ENDS */
	printf("server: FILE TRANSFER COMPLETE on socket %d\n",ssock);
	fclose(fp);
	//return(endexecution(option));
	//close(ssock);
	return;
	//return NULL;
}



/*
  TO TAKE CARE OF THE POSSIBILITY OF BUFFER LIMMITS IN THE KERNEL FOR THE
 SOCKET BEING REACHED (WHICH MAY CAUSE READ OR WRITE TO RETURN FEWER CHARACTERS
  THAN REQUESTED), WE USE THE FOLLOWING TWO FUNCTIONS */  
   
int readn(int sd,char *ptr,int size)
{
	int no_left,no_read;
	no_left = size;
	while (no_left > 0) 
	{ 
		no_read = read(sd,ptr,no_left);
		if(no_read <0)  return(no_read);
		if (no_read == 0) break;
		no_left -= no_read;
		ptr += no_read;
	}
	return(size - no_left);
}

int writen(int sd,char *ptr,int size)
{
	int no_left,no_written;
	no_left = size;
	while (no_left > 0) 
	{ 
		no_written = write(sd,ptr,no_left);
		if(no_written <=0)  return(no_written);
		no_left -= no_written;
		ptr += no_written;
	}
	return(size - no_left);
}
