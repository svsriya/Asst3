#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*
 * Setting up the server:
 *  1. getaddrinfo on itself
 *  2. make call to the socket
 *  3. make call to bind
 *  4. make call to listen
 *  5. call to accept (loop to accept multiple connections
*/

int main( int argc, char** argv ){
	
//	int port_num = argv[1];

	struct addrinfo server;
	struct addrinfo * results;
	server.ai_family = AF_INET;
	server.ai_socktype = SOCK_STREAM;
	server.ai_flags = AI_PASSIVE;
	
	int res =  getaddrinfo(NULL, argv[1], &server, &results);
	if(res!=0){
		printf("Error: %s\n", gai_strerror(res)); exit(2);
	}


	//setup socket
	int sockfd;
	if( (sockfd = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);
	}

	//setup bind
	if( bind(sockfd, results->ai_addr, results->ai_addrlen) == -1){	
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
		exit(2);
	}

	//call listen
	if( listen(sockfd, 10) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
		exit(2);
	}

	//call accept
	int cfd;
	printf("Searching for connection....\n");
	if( (cfd = accept(sockfd, (results->ai_addr), &(results->ai_addrlen))) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
		exit(2);	
	}else{
		printf("Client found!\n");

	}		
	
	//receive message from client
	char bufread [500];
	if( read(cfd, bufread, sizeof(bufread)) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
		exit(2);	
	}else{
		printf("Message sent by client: %s\n");
	}

	printf("Server disconnected from client.\n");
	
	close(cfd); close(sockfd);
	return 0; 
}
