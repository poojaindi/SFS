#include "server.h"

int multicasts(char * filename)
{ 

	struct in_addr localInterface;
	struct sockaddr_in groupSock;
	int sd;
	int datalen, bytes_read, i;
	char c;
	char content[512];
	FILE *fp;
	int fsize = 0;
	int num_blks, num_blks1;
	int num_last_blk, num_last_blk1;

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) 
	{
		perror("opening datagram socket");
		exit(1);
	}

	/*
	 * Initialize the group sockaddr structure with a
	 * group address of 225.1.1.1 and port 5555.
	 */
	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr("225.1.1.1");
	groupSock.sin_port = htons(5555);

	/*
	 * Disable loopback so you do not receive your own datagrams.
	 */
	/*  {
	    char loopch=0;

	    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP,
	    (char *)&loopch, sizeof(loopch)) < 0) {
	    perror("setting IP_MULTICAST_LOOP:");
	    close(sd);
	    exit(1);
	    }
	    }*/

	/*
	 * Set local interface for outbound multicast datagrams.
	 * The IP address specified must be associated with a local,
	 * multicast-capable interface.
	 NADDR_ANY);*/
	localInterface.s_addr = htonl (INADDR_ANY);
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface,
						sizeof(localInterface)) < 0) 
	{
		perror("setting local interface");
		exit(1);
	}

	/*
	 * Send a message to the multicast group specified by the
	 * groupSock sockaddr structure.
	 */
	/*
	   datalen = sizeof(databuf);
	   if (sendto(sd, databuf, datalen, 0,
	   (struct sockaddr*)&groupSock,
	   sizeof(groupSock)) < 0)
	   {
	   perror("sending datagram message");
	   }
	   printf("Sending done\n");*/
	if(sendto(sd, filename, strlen(filename), 0, (struct sockaddr*)
						&groupSock,sizeof(groupSock)) < 0)
	{
		perror("writing filename");
		exit(1);
	}
	if((fp = fopen(filename, "r")) == NULL)
	{
		perror("Opening file");
		exit(1);
	}

	while ((c = getc(fp)) != EOF) 
		fsize++;

	num_blks = fsize / MAXLINE;
	num_blks1 = htons(num_blks);
	num_last_blk = fsize % MAXLINE;
	num_last_blk1 = htons(num_last_blk);
	//  printf("Sending number of blocks %d\n",num_blks);
	sendto(sd,&num_blks1, sizeof(num_blks1), 0, (struct sockaddr*)&groupSock, 
						sizeof(groupSock));
	//  printf("Sending extra bytes %d\n",num_last_blk);
	sendto(sd, &num_last_blk1, sizeof(num_last_blk1), 0, (struct sockaddr*)&groupSock, 
						sizeof(groupSock));
	rewind(fp);

	for(i= 0; i < num_blks; i ++)
	{
		if((fread(content,sizeof(char),512,fp)) < 0)
			perror("reading block\n");
		sendto(sd, content, sizeof(content), 0, (struct sockaddr*)&groupSock,
						sizeof(groupSock));

	}
	if (num_last_blk > 0)
	{

		if((fread(content,sizeof(char),num_last_blk,fp)) < 0)
			perror("Reading last blocks");

		sendto(sd, content, sizeof(content), 0, (struct sockaddr*)&groupSock,
						sizeof(groupSock));
	}

	fclose(fp);
	num_blks = num_blks1 = 0;
	num_last_blk = num_last_blk1 = 0;
	fsize = 0;
	memset(content, 0, sizeof(content));
	//  memset(databuf, 0, sizeof(databuf));
	memset(filename, 0, sizeof(filename));
	printf("Done!!!\n");

}
