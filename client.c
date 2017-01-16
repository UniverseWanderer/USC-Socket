/*
** client.c -- the client to request computation
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

//Define the port number
#define DTCPPORT "25180"

//Max numbers we can get at once
#define MAXDATASIZE 1000 

//Define the command code which is the ASCII value of last letter of command
//Ex: min -- MINCODE='n' which is 110
#define MINCODE 110
#define MAXCODE 120
#define SUMCODE 109
#define SOSCODE 115

//Return the string expression of command
char *getCmd(int command);

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	int result[1];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET_ADDRSTRLEN];
	int command;
	int nums[MAXDATASIZE];
	memset(&nums, 0, sizeof nums);
	
	if (argc != 2) {
		fprintf(stderr,"usage: client command\n");
		exit(1);
	}
	command = argv[1][2];
	if (!(command==MINCODE || command==MAXCODE || command==SUMCODE || command==SOSCODE)){
		fprintf(stderr,"wrong command: command must be {min, max, sum, sos}\n");
		exit(1);
	}
	
	nums[0]=command;
	
	/**********************File*************************/
	FILE *fp;
	int num, ii=1;
	if((fp=fopen("nums.csv","r"))==NULL)
	{
		printf("file cannot be opened\n");
		exit(1);
	}    
	while (!feof (fp))
	{  
		fscanf (fp, "%d", &nums[ii++]); 
	}
	fclose (fp);
	
	/**********************Socket*************************/
	printf("The client is up and running.\n");
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	if ((rv = getaddrinfo("localhost", DTCPPORT, &hints, &servinfo)) != 0) {//from Beej's Guide
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	//Loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {//from Beej's Guide
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	
	freeaddrinfo(servinfo); 
	
	
	/**********************************************************/
	/**********************SendAWS*****************************/
	/**********************************************************/
		
	if ((numbytes = send(sockfd, (char*)nums, ii*sizeof(int), 0)) == -1) {
		perror("talker: send");
		exit(1);
	}	
	printf("The client has sent the reduction type %s to AWS.\n",getCmd(command));
	printf("The client has sent %d numbers to AWS\n",numbytes/sizeof(int)-1);
	
	
	/**********************************************************/
	/*********************ReceiveAWS***************************/
	/**********************************************************/
		
	if ((numbytes = recv(sockfd, result, sizeof(int), 0)) == -1) {
		perror("recv");
		exit(1);
	}		
	printf("The client has received reduction %s: %d\n",getCmd(command),result[0]);
	
	close(sockfd);
	
	return 0;
}


//Transfer number to specific reduction type
char *getCmd(int command){
	switch(command){
		case MINCODE:
			return "MIN";
			break;
		case MAXCODE:
			return "MAX";
			break;
		case SUMCODE:
			return "SUM";
			break;
		case SOSCODE:
			return "SOS";
			break;
		default:
			return "ERROR";
			break;
	}
}