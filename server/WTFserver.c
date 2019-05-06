#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "sendrcvfile.h"
#include "sendrcvfile.c"

#include "createprotocol.h"
#include "createprotocol.c"

#include "parseprotoc.c"
#include "parseprotoc.h"

#include "commands.c"
#include "commands.h"

#include "threadsetup.c"
//#include "threadsetup.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define BACKLOG 50

/*
 * Setting up the server:
 *  1. getaddrinfo on itself
 *  2. make call to the socket
 *  3. make call to bind
 *  4. make call to listen
 *  5. call to accept (loop to accept multiple connections
*/

/*globals*/
th_container threads [BACKLOG];
int numthreads;
pthread_t mainboi; 
int sockfd;
PROJECT * PROJECTS_LL = NULL;
pthread_mutex_t LL_lock = PTHREAD_MUTEX_INITIALIZER;


/*function prototypes*/
int charToInt( char* );
int checkoutProj(int);
int searchProj(char *);
int createProj(int);
int writeToSocket( char **, int);
int currver(int, int, char**);
int fetchHistory(int);

void * handleClient(void *);
void * handle_sigs(void *);

/*linked list related*/
PROJECT ** searchinLL(char **, PROJECT **);
void addToLL(char **);

PROJECT ** searchinLL(char ** projname, PROJECT ** projstruct){
	PROJECT * head = PROJECTS_LL;
	PROJECT * curr = head;

	while(curr!=NULL){
		printf("searching...\n");
		if(curr->projname == *projname){
			*projstruct = curr;
			printf("match found!\n");
			return projstruct;
		}else{
			curr=curr->next;
		}
	}
	return NULL;
}

void addToLL(char ** projname){
	printf(ANSI_COLOR_CYAN "IN ADDTOLL.\n" ANSI_COLOR_RESET);
	PROJECT * newproj = (PROJECT*)malloc(sizeof(PROJECT)*1); //create new LL
	newproj->proj_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER; //initialize projlock

	//initialize projname
	newproj->projname = (char*)malloc(sizeof(char)*(strlen(*projname) + 1));
	strcpy(newproj->projname, *projname); 

	//set numthreads to 0 and init lock
	newproj->num_threads = 0;
	newproj->numthread_lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

	//set destroy status to 0
	newproj->destroy = 0;
	
	//adding to LL here//
	if(PROJECTS_LL == NULL){
		printf("PROJECTS_LL is null\n");
		PROJECTS_LL = newproj;
	}else{
		newproj->next = PROJECTS_LL;
		PROJECTS_LL = newproj;
	}

	return;
}


void * handle_sigs(void * args){
	sig_waiter_args * swargs = (sig_waiter_args *)args;
	int signum;
	printf("hangle_sigs thread blocking till SIGINT becomes pending...\n");	

	if(sigwait(swargs->set, &signum)!= 0){
		fprintf(stderr, "Errno: %d Msg: %s LINE#: %d", errno, strerror(errno), __LINE__); exit(1);
	}

	printf("Caught SIGINT. Shutting down Server...\n");
	shutdown(swargs->sockfd, SHUT_RDWR);
	pthread_exit(NULL);
	//return;
}



void * handleClient(void * thr_cont){
	struct th_container * th_ctr = (struct th_container *)thr_cont;
	//int cfd = ctr->cfd;
	int cfd = (*th_ctr).cfd;	

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
	int rdres = 1;
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

	//LOCK LL
	
	pthread_mutex_lock(&LL_lock);
	if(strcmp(bufread, "checkout") == 0){
	
		int retval = checkoutProj(cfd);		
		//sleep(15);
		if(retval == -1){
			printf("Error: Project checkout failed.\n"); //exit(2);	
		}
	}else if(strcmp(bufread, "create") == 0){
		int retval = createProj(cfd);
		if(retval == -1){
			printf("Error: Project creation failed.\n"); //exit(2);
		}else if(retval == -3){
			printf("Project created in server, not sent to client. Client already posesses project.\n");
		}/*else add to LL*/

		
	}else if(strcmp(bufread, "currentversion") == 0){
		int retval = currver(cfd, 0, NULL);
		if(retval == -1){
			printf("Error: Fetching currentversion failed.\n");// exit(2);
		}
	}else if(strcmp(bufread, "history") == 0){
		int retval = fetchHistory(cfd);
		if(retval == -1){
			printf("Error: Failed to obtain project history.\n");// exit(2);
		}
	}else if( strcmp(bufread, "destroy") == 0){
		if( destroy(cfd) == -1 ){
			printf( "Error: failed to destroy the project.\n"); //exit(2);
		}
	}else{
		//while(1);
		printf("Error: invalid command.\n"); //exit(2);
	}	

	pthread_mutex_unlock(&LL_lock);
	//printf("Server disconnected from client.\n");
	free(bufread);
	close(cfd);

	(*th_ctr).is_done = 1;
	
	pthread_exit(NULL);  //this function always succeeds
	return;
}


int fetchHistory(int cfd){

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


	if( searchProj(bufread2) == -5){
		//createProtocol("Error: project not found.\n", cfd);
		char len[10];
		char * err = (char *)malloc(sizeof(char)* 27);
		strcat(err,  "Error: project not found.\n");
		sprintf(len, "%010d", strlen(err)); 
		int i = 0;
		int written;

		writeToSocket(&err, cfd);

		printf(ANSI_COLOR_YELLOW "err sent to client\n" ANSI_COLOR_RESET, len);
		free(bufread2);
		free(err);
		return -1;

	}else{
		char * patho = (char *)malloc(6 + 1 + strlen(bufread2) + 9 + 1);
		patho[0] = '\0';
		strcat(patho, "./root/");
		strcat(patho, bufread2);
		strcat(patho, "/history");
		char * history = (char*)malloc(sizeof(char)*8);
		strcpy(history, "history");				
		createProtocol(&patho, &history, &bufread2, cfd ); 	
		free(history);
		free(bufread2);
	}

}		

int currver(int cfd, int cs, char ** pn){
	char * bufread2;
	if(cs == 0){
		char buflen[10];
		if( read(cfd, buflen, sizeof(buflen)) == -1){
			printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
			return -1;	
		}

		int len = charToInt((char*)buflen);	
		printf("Number of bytes_projname being recieved: %d\n", len);

		//get projname from client
		bufread2 = (char*)malloc( len + 1 );	
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

	}else{
		bufread2 = *pn;
	}

	//lock bigLL
	//pthread_mutex_lock(&LL_lock);
/*	PROJECT ** projstruct;
	PROJECT * astruct = (PROJECT*)malloc(sizeof(PROJECT));
	//search for Proj in LL
	if((projstruct = searchinLL(&bufread2, &astruct)) == NULL){
		printf("Error: project not found.\n");
		//unlock bigLL mutex
		char len[10];
		char * err = (char *)malloc(sizeof(char)* 27);
		strcat(err,  "Error: project not found.\n");
		sprintf(len, "%010d", strlen(err)); 
		int i = 0;
		//int written;

		writeToSocket(&err, cfd);
		
		pthread_mutex_unlock(&LL_lock);
		return -1;
	}else{
		//unlock big LL
		pthread_mutex_unlock(&LL_lock);
		//lock struct
		pthread_mutex_lock(&((*projstruct)->proj_lock));
		
		//lock numthread_lock
		pthread_mutex_lock(&((*projstruct)->numthread_lock));
			(*projstruct)->num_threads = ((*projstruct)->num_threads) + 1;
		pthread_mutex_unlock(&((*projstruct)->numthread_lock)); //unlock numthread lock after incrementing

		if((*projstruct)->destroy == 1){
			printf("Error: project does not exist.\n");
			pthread_mutex_unlock(&((*projstruct)->proj_lock));
			char len[10];
			char * err = (char *)malloc(sizeof(char)* 27);
			strcat(err,  "Error: project not found.\n");
			sprintf(len, "%010d", strlen(err)); 
			int i = 0;
			//int written;

			writeToSocket(&err, cfd);
			
			//pthread_mutex_unlock(&((*projstruct)->proj_lock));
			return -1;
		}

	}
	free(astruct);*/
	if( searchProj(bufread2) == -5){
		//createProtocol("Error: project not found.\n", cfd);
		char len[10];
		char * err = (char *)malloc(sizeof(char)* 27);
		strcat(err,  "Error: project not found.\n");
		sprintf(len, "%010d", strlen(err)); 
		int i = 0;
		int written;

		writeToSocket(&err, cfd);
		
		printf(ANSI_COLOR_YELLOW "err sent to client\n" ANSI_COLOR_RESET, len);
		if(cs==0){
			free(bufread2);
		}
		free(err);
		return -1;

	}else{
	
		//read in .Manifest and write2socket

		char * patho = (char *)malloc(6 + 1 + strlen(bufread2) + 9 + 1);
		patho[0] = '\0';
		strcat(patho, "./root/");
		strcat(patho, bufread2);
		strcat(patho, "/.Manifest");
		struct stat fstat;
	
		int manf;

		if( (manf = open(patho, O_RDONLY)) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); exit(2);
		}

		if(stat(patho, &fstat) == -1){
			(ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);
		}
	
		char * manbf = malloc(fstat.st_size+1);
			manbf[0] = '\0';
	
		if( read(manf, manbf, fstat.st_size) == -1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);
		}

//		writeToSocket(&manbf, cfd);
		char * currver = (char*)malloc(sizeof(char)*8);
		strcpy(currver, "currver");				
		createProtocol(&patho, &currver, &bufread2, cfd ); 
		free(currver);	
		if(cs==0){
			free(bufread2); 
		}
		free(patho);
		free(manbf);
		close(manf);
		//return 0;
	}

/*	pthread_mutex_lock(&((*projstruct)->numthread_lock));
		(*projstruct)->num_threads = ((*projstruct)->num_threads) - 1;
	pthread_mutex_unlock(&((*projstruct)->numthread_lock));*/
	return 0;
}


int writeToSocket(char ** buf, int cfd){
	char len[10];
	char * msg = *buf;
	sprintf(len, "%010d", strlen(*buf)); 
	int i = 0;
	int written;

	while( i != strlen(len)){
		if( (written = write(cfd, len+i, strlen(len)-i)) == -1){
        	       	printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        	        exit(2);
		}
		i+=written;
	}	
	
	printf(ANSI_COLOR_YELLOW "Bytes buf sent to client\n" ANSI_COLOR_RESET);


	i=0;
	while( i!= strlen(*buf)){
		if( (written = write(cfd, (*buf)+i, strlen(*buf)-i)) == -1){
               		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                	exit(2);
		}	
		i+=written;
	}

	printf(ANSI_COLOR_YELLOW "buf sent to client\n" ANSI_COLOR_RESET, len);
	//ifree(bufread2);	


}


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

	/*pthread_mutex_lock(&LL_lock);
	PROJECT ** projstruct;
	PROJECT * astruct = (PROJECT*)malloc(sizeof(PROJECT));
	if((projstruct = searchinLL(&bufread2, &astruct)) == NULL){
		addToLL(&bufread2);	
		pthread_mutex_unlock(&LL_lock);	
	}else{
		//acquire lock
		pthread_mutex_unlock(&LL_lock);
		pthread_mutex_lock(&((*projstruct)->proj_lock));
		
		printf("Error: project already exists.\n");
	//	writeToSocket(&err, cfd);	
	//	pthread_mutex_unlock(&((*projstruct)->proj_lock));
		//return -1;
		

	}
	free(astruct);
	*/

	//search to see if proj already exists
	
	if( searchProj(bufread2) != -5){
		char len[10];
		char * err = (char *)malloc(sizeof(char)*6);
		strcat(err, "Error");
		sprintf(len, "%010d", strlen(err)); 
	
		writeToSocket(&err, cfd);
		//pthread_mutex_unlock(&((*projstruct)->proj_lock));
		printf(ANSI_COLOR_YELLOW "err sent to client\n" ANSI_COLOR_RESET, len);
		free(bufread2);	 free(err);

	}else{ //create the project
		printf(ANSI_COLOR_CYAN "creating project...\n" ANSI_COLOR_RESET);
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
		close(hisfd); close(manfd);// closedir(bufread2);
		//char * patho = (char *)malloc(6 + 1 + strlen(bufread2) + 1);
		//snprintf(path, strlen(path), "./root/%s\n", proj);
		//strcat(patho, "./root/");
		//strcat(patho, bufread2);	
		

		//addToLL(&bufread2);
		
		//write to client -- "Project created in server" -- HANDSHAKE
		char * handshake0 = (char*)malloc(sizeof(char)*28);
		handshake0[0] = '\0';
		strcat(handshake0, "Project created in Server.\n");
		writeToSocket(&handshake0, cfd);

		//CREATE protocol only if receive OK from client
		char handlen[10];

	//	int rdr = 1;
	//		int p = 0;
		
		if(read(cfd, handlen, sizeof(handlen)) == -1){
			printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
			return -1;	
		}
		
		int hlen = charToInt((char*)handlen);	
		printf("Number of bytes_hand1 being recieved: %d\n", hlen);
	
		//get projname from client
		char* handshake1 = (char*)malloc( hlen + 1 );	
		handshake1[0] = '\0';
		int rdres = 1;
		int i = 0;
		while( rdres > 0 ){
			rdres = read( cfd, handshake1+i, hlen-i );
			printf("rdres: %d\n", rdres);
			if( rdres == -1 ){	
				printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                		return -1;
			}
			i += rdres;
		}	
		handshake1[hlen] = '\0';	
		printf( "handshake1 received from client: %s\n", handshake1);

		if(strcmp(handshake1, "OK") == 0){
			char * create = (char*)malloc(sizeof(char)*7);
			strcpy(create, "create");				
			createProtocol(&path, &create, &bufread2, cfd ); /* creates and send protocol to client */
			free(create);
		}else{

			free(handshake0); free(handshake1);
			free(bufread2);
			return -1;
		}

		free(handshake0); free(handshake1);
		free(bufread2); //free(patho);
		//printf(ANSI_COLOR_CYAN"unlocking project mutex now.\n" ANSI_COLOR_RESET);	
		//pthread_mutex_unlock(&((*projstruct)->proj_lock));
		return 0;
	}

}


int searchProj(char * proj){
	DIR * dirp;
	char * path = (char *)malloc(6 + 1 + strlen(proj) + 1);
	//snprintf(path, strlen(path), "./root/%s\n", proj);
	path[0] = '\0';
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

		char * err = (char *)malloc(sizeof(char)* 27);
		strcat(err,  "Error: project not found.\n");
		sprintf(len, "%010d", strlen(err)); 
		int i = 0;
		int written;

		writeToSocket(&err, cfd);
		
		printf(ANSI_COLOR_YELLOW "err sent to client\n" ANSI_COLOR_RESET, len);
		free(bufread2);
		free(err);
		return -1;
	

	}else{
		char * patho = (char *)malloc(6 + 1 + strlen(bufread2) + 1);
	//snprintf(path, strlen(path), "./root/%s\n", proj);
		strcat(patho, "./root/");
		strcat(patho, bufread2);

		//printf("calling createprotoc\n");
		char * checkout = (char*)malloc(sizeof(char)*9);
		strcpy(checkout, "checkout");				
		createProtocol(&patho, &checkout, &bufread2, cfd ); /* creates and send protocol to client */
		free(checkout);
		free(bufread2); free(patho);
	}

//	free(bufread2);
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
	//int sockfd;
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
	if( listen(sockfd, BACKLOG) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__); 
		exit(2);
	}
	
	/*obtain ID of main thread*/
	mainboi = pthread_self();

	/*setup sigset*/	
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);

	if(pthread_sigmask(SIG_SETMASK, &sigset, NULL)!=0){
		fprintf(stderr, "Errno: %d Message: %s LINE#: %d\n", errno, strerror(errno), __LINE__);
		close(sockfd);
		exit(2);
	}

	pthread_t sigthread;
	sig_waiter_args arrgs;
	arrgs.set = &sigset;
	arrgs.sockfd = sockfd;
	if ((pthread_create(&sigthread, NULL, handle_sigs, (void*)(&arrgs))) !=0){
		fprintf(stderr, "Errno: %d Message: %s LINE#: %d\n", errno, strerror(errno), __LINE__);
		close(sockfd);
		exit(2);
	}
	printf("Sigint manager thread now running.\n");

	//call accept
	int cfd;
	printf("Searching for connection....\n");

	int ap = 1;

	int p;
	for(p=0; p<BACKLOG;p++){
		threads[p].is_done = -1;
	}
 
	numthreads = 0;
	while(ap == 1){
		if( (cfd = accept(sockfd, (results->ai_addr), &(results->ai_addrlen))) == -1){
			/*printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);*/ 
			ap=0;
			
			//pthread_cancel(thread_1);
			//exit(2);	
		}else{
			printf( ANSI_COLOR_YELLOW "Client found!\n" ANSI_COLOR_RESET );
	
			int i;
		
			int in_use=BACKLOG;
			while(in_use == BACKLOG){
				in_use = 0;
				for(i=0; i<BACKLOG; i++){
					if(threads[i].is_done == 1){
						//cont->thread_place = i;
						//cancel&join
						printf("position in th_container: %d\n", i);
				//		void * result;
						pthread_cancel((threads[i]).thread_id);
						//	printf(ANSI_COLOR_RED "Pthread_cancel failed.  LINE: %d\n" ANSI_COLOR_RESET, __LINE__); //exit(3);
						
						if( (pthread_join(threads[i].thread_id, NULL))!=0){
							printf(ANSI_COLOR_RED "Pthread_join failed.  LINE: %d\n" ANSI_COLOR_RESET, __LINE__); //exit(3);
						}
						numthreads--;
						//threads[i].is_done == -1;	//unnecessary?
					}

					if(threads[i].is_done == 1 || threads[i].is_done == -1){	
						threads[i].is_done=0;
						threads[i].cfd = cfd;
						printf("placing thread in threads[%d]\n", i);
						if((pthread_create(&(threads[i].thread_id), NULL, handleClient, (void*)&(threads[i]))) !=0){
							printf(ANSI_COLOR_RED "Error: pthread_create failed.  LINE: %d\n" ANSI_COLOR_RESET, __LINE__); exit(3); 
						}
						numthreads++;
						printf("numthreads: %d\n", numthreads);			
						break;
					}
					in_use++;			
				}
				if(in_use==BACKLOG){
					pthread_yield();
				}
			}
		}		
	}
	printf("Main broken out of accept loop\n");
	int w;
	for(w=0; w<BACKLOG; w++){
		if(threads[w].is_done != -1){
			if( (pthread_join(threads[w].thread_id, NULL))==0){
				printf("Joined thread %d\n", w); //exit(3);
			}else{					
				printf("Join failed on thread %d\n", w); //exit(3);
			}
		}
	}
	//cancel and join on sigthread
	pthread_cancel(sigthread);
	pthread_join(sigthread, NULL);
	printf("sigthread cleaned up.\n");
	close(sockfd);
	return 0;
}
