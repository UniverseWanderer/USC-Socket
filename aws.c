/*
** aws.c -- the Amazon Web Services server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>

//Define the port number
#define DUDPPORT "24180" 
#define DTCPPORT "25180"
#define AUDPPORT "21180" 
#define BUDPPORT "22180" 
#define CUDPPORT "23180" 

//Max numbers we can get at once
#define MAXBUFLEN 4000

//Max waiting queue length
#define BACKLOG 10

//Define the command code which is the ASCII value of last letter of command
//Ex: min -- MINCODE='n' which is 110
#define MINCODE 110
#define MAXCODE 120
#define SUMCODE 109
#define SOSCODE 115

//Return the string expression of command
char *getCmd(int command);

//Handle the event
void sigchld_handler(int s);

//Consult 3 servers to get reduction result
int consultServer(int nums[], int numbers);

//Combine the results from servers 
int combineData(int command, int ra, int rb, int rc);

int main(void)
{
	int sockfdUDP,sockfdTCP,new_fd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	int buf[MAXBUFLEN];
	socklen_t addr_len,sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET_ADDRSTRLEN];
	
	/**********************Socket*************************/
	printf("The AWS is up and running.\n");	
	
	//Build socket
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	
	if ((rv = getaddrinfo(NULL, DTCPPORT, &hints, &servinfo)) != 0) {//from Beej's Guide
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	//Loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {//from Beej's Guide
		if ((sockfdTCP = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("aws: socket");
			continue;
		}
		
		if (setsockopt(sockfdTCP, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("aws: setsockopt");
			exit(1);
		}
		if (bind(sockfdTCP, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfdTCP);
			perror("aws: bind");
			continue;
		}		
		break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "aws: failed to bind socket\n");
		return 2;
	}
	
	freeaddrinfo(servinfo);
	
	//Listen to the socket
	if (listen(sockfdTCP, BACKLOG) == -1) {
		perror("aws: listen");
		exit(1);
	}
	
	//Reap all dead processes //from Beej's Guide
	sa.sa_handler = sigchld_handler; 
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("aws: sigaction");
		exit(1);
	}
	
	//Main accept() loop
	while(1) { 
		sin_size = sizeof their_addr;
		new_fd = accept(sockfdTCP, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			continue;
		}		
		
		//This is the child process
		if (!fork()) { 
			close(sockfdTCP); //Child doesn't need the listener
			
			/**********************ReceiveFrom*************************/
			//Receive data from client
			if ( (numbytes = recv(new_fd, buf, MAXBUFLEN, 0))== -1)
				perror("receive");			
			
			printf("The AWS has received %d numbers from the client using TCP over port %s \n",numbytes/sizeof(int)-1,DTCPPORT);
			
			int command=buf[0];
			/*printf("Debug: Cmd: %s.\n",getCmd(command));
			
			printf("Debug: packet contains ");
			for(int i=1; i<numbytes/sizeof(int); i++)
			{
				printf("%d ",buf[i]);
			}
			printf("\n");		
			*/
			
			/********************ConsultServer*********************/
			int result[1];
			result[0] = consultServer(buf,numbytes/sizeof(int));
			
			printf("The AWS has successfully finished the reduction %s: %d\n",getCmd(command),result[0]);
			
			
			/**********************SendBack************************/
			//Send the result back to client			
			if (send(new_fd, (char*)result, sizeof(int), 0) == -1)
				perror("send");
			
			printf("The AWS has successfully finished sending the reduction value to client\n");
			
			close(new_fd);
			exit(0);
		}
		
		close(new_fd);
	}		
	
	return 0;
}



int consultServer(int nums[],int numbers){	
	int sockfdUDP;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	int bufA[MAXBUFLEN],bufB[MAXBUFLEN],bufC[MAXBUFLEN];
	int ra[1], rb[1], rc[1];
	socklen_t addr_len;
	char s[INET_ADDRSTRLEN];
	int command = nums[0];
	
	/**********************Socket*************************/
	//Build socket
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; 
	
	if ((rv = getaddrinfo(NULL, DUDPPORT, &hints, &servinfo)) != 0) {//from Beej's Guide
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	//Loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {//from Beej's Guide
		if ((sockfdUDP = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("UDP: socket");
			continue;
		}
		
		if (bind(sockfdUDP, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfdUDP);
			perror("UDP: bind");
			continue;
		}		
		break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "UDP: failed to bind socket\n");
		return 2;
	}
	
	freeaddrinfo(servinfo);
	
	/**********************PackABC*****************************/
	//Reserve the command
	bufA[0]=command;
	bufB[0]=command;
	bufC[0]=command;
	
	//Seperate the data
	int k=1,i=1;
	for(i; i<numbers; k++){
		bufA[k]=nums[i++];
		bufB[k]=nums[i++];
		bufC[k]=nums[i++];
	}
	
	/**********************************************************/
	/*************************A********************************/
	/**********************************************************/

	/**********************SendtoA*****************************/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	if ((rv = getaddrinfo("localhost", AUDPPORT, &hints, &servinfo)) != 0) {//from Beej's Guide
		fprintf(stderr, "getaddrinfoA: %s\n", gai_strerror(rv));
		return 3;
	}
	
	//Loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if (p != NULL)
			break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "ServerA: failed to create socket\n");
		return 4;
	}
	
	freeaddrinfo(servinfo);
	
	//Send data to serverA
	if ((numbytes = sendto(sockfdUDP, (char*)bufA, k*sizeof(int), 0,
			p->ai_addr, p->ai_addrlen)) == -1) {
		perror("ServerA: sendto");
		exit(1);
	}	
	printf("The AWS has sent %d numbers to BackendServer A\n", numbytes/sizeof(int)-1);
	
	/**********************ReceivefromA**************************/		
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfdUDP, ra, sizeof(int), 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	
	/**********************************************************/
	/*************************B********************************/
	/**********************************************************/
	
	/**********************SendtoB*****************************/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	if ((rv = getaddrinfo("localhost", BUDPPORT, &hints, &servinfo)) != 0) {//from Beej's Guide
		fprintf(stderr, "getaddrinfoB: %s\n", gai_strerror(rv));
		return 3;
	}
	
	//Loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if (p != NULL)
			break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "ServerB: failed to create socket\n");
		return 4;
	}
	
	freeaddrinfo(servinfo);
	
	//Send data to serverB
	if ((numbytes = sendto(sockfdUDP, (char*)bufB, k*sizeof(int), 0,
			p->ai_addr, p->ai_addrlen)) == -1) {
		perror("ServerB: sendto");
		exit(1);
	}	
	printf("The AWS has sent %d numbers to BackendServer B\n", numbytes/sizeof(int)-1);
	
	/**********************ReceivefromB**************************/		
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfdUDP, rb, sizeof(int), 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	
	/**********************************************************/
	/*************************C********************************/
	/**********************************************************/
	
	/**********************SendtoC*****************************/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	if ((rv = getaddrinfo("localhost", CUDPPORT, &hints, &servinfo)) != 0) {//from Beej's Guide
		fprintf(stderr, "getaddrinfoC: %s\n", gai_strerror(rv));
		return 5;
	}
	
	//Loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if (p != NULL)
			break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "ServerC: failed to create socket\n");
		return 6;
	}
	
	freeaddrinfo(servinfo);
	
	//Send data to serverC
	if ((numbytes = sendto(sockfdUDP, (char*)bufC, k*sizeof(int), 0,
			p->ai_addr, p->ai_addrlen)) == -1) {
		perror("ServerC: sendto");
		exit(1);
	}	
	printf("The AWS has sent %d numbers to BackendServer C\n", numbytes/sizeof(int)-1);
	
	/**********************ReceivefromC**************************/		
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfdUDP, rc, sizeof(int), 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	
	// UDP end 
	close(sockfdUDP);
	
	/**********************Combine*************************/		
	//Recieve from backend server
	printf("The AWS received reduction result of %s from BackendServer A using UDP over port %s and it is %d\n",getCmd(command),DUDPPORT,ra[0]);	
		
	//Recieve from backend server
	printf("The AWS received reduction result of %s from BackendServer B using UDP over port %s and it is %d\n",getCmd(command),DUDPPORT,rb[0]);	
	
	//Recieve from backend server
	printf("The AWS received reduction result of %s from BackendServer C using UDP over port %s and it is %d\n",getCmd(command),DUDPPORT,rc[0]);	

	//Combine all result 	
	int result = combineData(command, ra[0], rb[0], rc[0]);

	//Return the result to client
	return result;
}

//Combine the result from 3 servers
int combineData(int command, int ra, int rb, int rc)
{
	switch(command){
		case MINCODE:	//"MIN"
		{
			int temp= ra<rb ? ra : rb;
			return temp<rc ? temp : rc;
		}
		case MAXCODE:	//"MAX"
		{
			int temp= ra>rb ? ra : rb;
			return temp>rc ? temp : rc;
		}
		case SUMCODE:	//"SUM"
			return ra+rb+rc;
		case SOSCODE:	//"SOS"
			return ra+rb+rc;
		default:
			return 0;
	}
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

void sigchld_handler(int s)//from Beej's Guide
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);
	
	errno = saved_errno;
}