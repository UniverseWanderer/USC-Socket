/*
** serverA.c -- serverA, one of the backend servers to process the data.
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

//Define the port number
#define AUDPPORT "21180" 
#define DUDPPORT "24180"

//Max numbers we can get at once 334*4 bytes
#define MAXBUFLEN 1336

//Define the command code which is the ASCII value of last letter of command
//Ex: min -- MINCODE='n' which is 110
#define MINCODE 110
#define MAXCODE 120
#define SUMCODE 109
#define SOSCODE 115

//Return the string expression of command
char *getCmd(int command);

//Process data based on specific command
int prcData(int buf[], int numbers);

//Get sockaddr, IPv4
void *get_in_addr(struct sockaddr *sa)//from Beej's Guide
{
	return &(((struct sockaddr_in*)sa)->sin_addr);
}

int main(void)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	int buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET_ADDRSTRLEN];
	int result[1];
	
	/**********************Socket*************************/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; 
	
	if ((rv = getaddrinfo(NULL, AUDPPORT, &hints, &servinfo)) != 0) {//from Beej's Guide
		fprintf(stderr, "getaddrinfoA: %s\n", gai_strerror(rv));
		return 1;
	}
	
	//Loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {//from Beej's Guide
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("A: socket");
			continue;
		}
	
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("A: bind");
			continue;
		}		
		break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "A: failed to bind socket\n");
		return 2;
	}
	
	freeaddrinfo(servinfo);
	
	printf("The Server A is up and running using UDP on port %s.\n",AUDPPORT);
	
	while(1){
	/*******************Receivefrom***********************/
	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN, 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}	
	
	printf("The Server A has received %d numbers\n",numbytes/sizeof(int)-1);

	
	/********************Calculate******************************/
	//Do reduction
	result[0]=prcData(buf,numbytes/sizeof(int));
	

	/***********************Sendto*******************************/
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	if ((rv = getaddrinfo(inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s), DUDPPORT, &hints, &servinfo)) != 0) {//from Beej's Guide
		fprintf(stderr, "getaddrinfoD: %s\n", gai_strerror(rv));
		return 3;
	}
	
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL ; p = p->ai_next) {
		if(p != NULL)
			break;
	}
	
	if (p == NULL) {
		fprintf(stderr, "D: failed to create socket\n");
		return 4;
	}
	
	freeaddrinfo(servinfo);

	//Send to aws
	if ((numbytes = sendto(sockfd, (char*)result, sizeof(int), 0,
			p->ai_addr, p->ai_addrlen)) == -1) {
		perror("D: sendto");
		exit(1);
	}		

	printf("The Server A has successfully finished sending the reduction value to AWS server.\n");
	
	}
	
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

//Process the data according to command
int prcData(int buf[], int numbers){
	int command = buf[0];
	int result,i;

	switch(command){
		case MINCODE:
			{	//"MIN"
				int temp=65535;
				for(i=1; i<numbers; i++){
					temp= buf[i]<temp ? buf[i] : temp;
				}
				result = temp;
			}
			break;
		case MAXCODE:
			{	//"MAX"
				int temp=-65535;
				for(i=1; i<numbers; i++){
					temp= buf[i]>temp ? buf[i] : temp;
				}
				result = temp;
			}
			break;
		case SUMCODE:
			{	//"SUM"
				int temp=0;
				for(i=1; i<numbers; i++){
					temp += buf[i];
				}
				result = temp;
			}
			break;
		case SOSCODE:
			{	//"SOS"
				int temp=0;
				for(i=1; i<numbers; i++){
					temp += buf[i]*buf[i];
				}
				result = temp;
			}
			break;
		default:
			result = 0;
			break;
	}
	
		
	printf("The Server A has successfully finished the reduction %s: %d\n",getCmd(command),result);

	return result;
}