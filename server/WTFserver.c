#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "sendrcvfile.h"
#include "sendrcvfile.c"

#include "createprotocol.h"
#include "createprotocol.c"


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

int charToInt( char* );
int checkoutProj(int);
int searchProj(char *);
int createProj(int);

int createProj(int cfd){
	char buflen[10];
	if( read(cfd, buflen, sizeof(buflen)) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
		return -1;	
	}

	int len = charToInt((char*)buflen);	
	printf("Number of bytes_projname being recieved: %d\n", len);

	//get projname from client
	char* bufread2 = (char*)malloc( len + 1 );	
//	bufread[0] = '\0';
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){
		rdres = read( cfd, bufread2+i, len-i );
		printf("rdres: %d\n", rdres);
		if( rdres == -1 ){	
			printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                	return -1;
		}
		i += rdres;
	}	
	bufread2[len] = '\0';	
	printf( "projname received from client: %s\n", bufread2);


	//search to see if proj already exists
	
	if( searchProj(bufread2) != -5){
		char len[10];
		char * err = "Error";
		sprintf(len, "%010d", strlen(err)); 
		int i = 0;
		int written;

		while( i != strlen(len)){
			if( (written = write(cfd, len+i, strlen(len)-i)) == -1){
        	       		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        	        	exit(2);
			}
			i+=written;
		}	
	
		printf(ANSI_COLOR_YELLOW "%s bytes to read send to client\n" ANSI_COLOR_RESET, len);


		i=0;
		while( i!= strlen(err)){
			if( (written = write(cfd, err+i, strlen(err)-i)) == -1){
               			printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                		exit(2);
			}	
			i+=written;
		}

		printf(ANSI_COLOR_YELLOW "err sent to client\n" ANSI_COLOR_RESET, len);
		free(bufread2);	

	}else{ //create the project
		char * path = (char *)malloc(6 + 1 + strlen(bufread2) + 1);
		//snprintf(path, strlen(path), "./root/%s\n", proj);
		strcat(path, "./root/");
		strcat(path, bufread2);

		//change dir into root
		char * maindir = malloc(sizeof(char)*10000); maindir[0]= '\0';
		maindir = getcwd(maindir, 10000);
		int len = strlen(maindir);
		maindir[len]= '\0';
		printf("maindir: %s\n", maindir);

		if(maindir == NULL){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);
		}

		chdir("./root");
		if(mkdir(bufread2, S_IRUSR | S_IWUSR | S_IXUSR) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);
		}

		chdir(maindir); //free(maindir);
		chdir(path);
		
		printf("current path: %s\n", path);
		
		//create manifest
		int manfd;
		if( (manfd = open(".Manifest", O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
				exit(2);	
		}

		int man_init;
		char * headers = "1\nFilename\tVersion\tOn Serv\tRemoved\tHash";
		
		if( (man_init = write(manfd, headers, strlen(headers))) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);	
		} 		
	

		//create history
		int hisfd;		
		if( (hisfd = open("history", O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
				exit(2);	
		}

		int his_init;
		char * headers1 = "create\n0";
		
		if( (his_init = write(hisfd, headers1, strlen(headers1))) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);	
		} 		

		//send project to client via createProtocol
		if(chdir(maindir) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);		
		}


		free(maindir);
		//char * patho = (char *)malloc(6 + 1 + strlen(bufread2) + 1);
		//snprintf(path, strlen(path), "./root/%s\n", proj);
		//strcat(patho, "./root/");
		//strcat(patho, bufread2);		
		createProtocol(&path, cfd ); /* creates and send protocol to client */
		free(bufread2); //free(patho);

	}
	
}


int searchProj(char * proj){
	DIR * dirp;
	char * path = (char *)malloc(6 + 1 + strlen(proj) + 1);
	//snprintf(path, strlen(path), "./root/%s\n", proj);
	strcat(path, "./root/");
	strcat(path, proj);
	printf("PATHSearch: %s\n", path);

//	DIR * dirp;
	if((dirp = opendir(path)) == NULL){
		free(path);
		return -5;
	}
	
	free(path);
	//printf("done search\n");
	return 0;	

}

int checkoutProj(int cfd){

	//get bytesname
	char buflen[10];
	if( read(cfd, buflen, sizeof(buflen)) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
		return -1;	
	}

	int len = charToInt((char*)buflen);	
	printf("Number of bytes_projname being recieved: %d\n", len);

	//get projname from client
	char* bufread2 = (char*)malloc( len + 1 );	
//	bufread[0] = '\0';
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){
		rdres = read( cfd, bufread2+i, len-i );
		printf("rdres: %d\n", rdres);
		if( rdres == -1 ){	
			printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                	return -1;
		}
		i += rdres;
	}	
	bufread2[len] = '\0';	
	printf( "projname received from client: %s\n", bufread2);
	// call method to find the file
//	tarfile( bufread );
//	strcat( bufread, ".tgz\0" );
//
//must search for project here

	if( searchProj(bufread2) == -5){
		//createProtocol("Error: project not found.\n", cfd);
		char len[10];
		char * err = "Error: project not found.\n";
		sprintf(len, "%010d", strlen(err)); 
		int i = 0;
		int written;

		while( i != strlen(len)){
			if( (written = write(cfd, len+i, strlen(len)-i)) == -1){
        	       		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        	        	exit(2);
			}
			i+=written;
		}	
	
		printf(ANSI_COLOR_YELLOW "%s bytes to read send to client\n" ANSI_COLOR_RESET, len);


		i=0;
		while( i!= strlen(err)){
			if( (written = write(cfd, err+i, strlen(err)-i)) == -1){
               			printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                		exit(2);
			}	
			i+=written;
		}

		printf(ANSI_COLOR_YELLOW "err sent to client\n" ANSI_COLOR_RESET, len);
		//free(bufread2);	
		return -1;
	

	}else{
		char * patho = (char *)malloc(6 + 1 + strlen(bufread2) + 1);
	//snprintf(path, strlen(path), "./root/%s\n", proj);
		strcat(patho, "./root/");
		strcat(patho, bufread2);

		//printf("calling createprotoc\n");	
		createProtocol(&patho, cfd ); /* creates and send protocol to client */
		free(bufread2); free(patho);
	}

	free(bufread2);
	return 0;
}

int charToInt( char* numArr ){
	// used to decipher how many bytes are being sent so string issues stop arising
	// number will end with ":" to tell the user that the number ended
	int i;
	for( i = 0;  i<strlen(numArr); i++ ); // i = number of digits in the number;
	char* num = (char*)malloc( i + 2 );
	int j;
	for( j = 0; j < i; j++ )
		num[j] = numArr[j];
	num[i] = '\0';
	int bytes = atoi(num);
	free( num );
	return bytes; 
}

int main( int argc, char** argv ){
	
//	int port_num = argv[1];

	struct addrinfo server;
	struct addrinfo * results;
	int rdres;
	
	memset(&server, 0, sizeof(struct addrinfo));

	server.ai_family = AF_INET;
	server.ai_socktype = SOCK_STREAM;
	server.ai_flags = AI_PASSIVE;	
	
	int res =  getaddrinfo(NULL, argv[1], &server, &results);
	if(res!=0){
		printf("Error: %s\n", gai_strerror(res)); exit(2);
	}

	//check to see if root dir exists. if yes, create. if not, dont create and sets errno
	//INITIAL BOOTUP of WTFserver CREATES EMPTY ROOT
	if(mkdir("root", S_IRUSR | S_IWUSR | S_IXUSR) == -1){
		printf("./root dir exists!\n");
	}else{
		printf("./root dir created!\n");
	}

	//setup socket
	int sockfd;
	if( (sockfd = socket(results->ai_family, results->ai_socktype, results->ai_protocol)) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);
	}

	int enable = 1;
	if( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1 ){
         	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
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
		printf( ANSI_COLOR_YELLOW "Client found!\n" ANSI_COLOR_RESET );

	}		
	
	//receive number of bytes in message from client
	//
	//CMD setup : i.e. checkout
	char buflen[10];
	if( read(cfd, buflen, sizeof(buflen)) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
		exit(2);	
	}

	int len = charToInt((char*)buflen);	
	printf("Number of bytes_cmd being recieved: %d\n", len);
	//get cmd from client
	char* bufread = (char*)malloc( len + 1 );	
//	bufread[0] = '\0';
	rdres = 1;
	int i = 0;
	while( rdres > 0 ){
		rdres = read( cfd, bufread+i, len-i );
		printf("rdrescmd: %d\n", rdres);
		if( rdres == -1 ){	
			printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                	exit(2);
		}
		i += rdres;
	}	
	bufread[len] = '\0';	
	printf( "cmd received from client: %s\n", bufread );
	//free(bufread);
	
	/* if bufread  = checkout. do below checkoutproj(proj, cfd) */
	if(strcmp(bufread, "checkout") == 0){
		int retval = checkoutProj(cfd);		
		if(retval == -1){
			printf("Error: Project checkout failed.\n"); exit(2);	
		}
	}else if(strcmp(bufread, "create") == 0){
		int retval = createProj(cfd);
		if(retval == -1){
			printf("Errot: Project creation failed.\n"); exit(2);
		}
		
	}	
		
	//if bufread == checkout then create protocol
//	createGzip();
//	sendProtocol("./protocol.txt", cfd);
//	destroyProtocolFile();	

	//createGzip();

	printf("Server disconnected from client.\n");
	free(bufread);
	close(cfd); close(sockfd);
	return 0; 
}
