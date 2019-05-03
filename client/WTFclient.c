#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#include "parseprotoc.c"
#include "parseprotoc.h"
#include "clientcommands.c"
#include "clientcommands.h"
#include "commitupdate.h"
#include "commitupdate.c"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/* To set up client:
 *  1. getaddrinfo on server
 *  2. make call to socket
 *  3. make call to connect
*/
void configure( char*, char* );
int charToInt( char* );
int checkout (char **, int);
int create (char **, int);
int currentversion(char **, int, int);
int history (char **, int);

int history(char ** projname, int sd){
	char * cmd = (char*)malloc(sizeof(char)*8);
	strcpy(cmd, "history");
	writeToSocket(&cmd, sd);
	free(cmd);


	char *pname = (char*)malloc(sizeof(char)* (strlen(*projname)+1));
	pname[0]='\0';
	strcpy(pname, *projname);
	writeToSocket(&pname, sd);
	free(pname);

	char buf2[10];
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){	
			rdres = read( sd, buf2+i, 10-i );
			printf( "number of bytes read: %d\n", rdres );
			if( rdres == -1 ){	
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        	close(sd); return -1;
			}
			i += rdres;
	}
	int bufflen = charToInt( (char*)buf2 );
	printf( ANSI_COLOR_MAGENTA "Number of bytes being recieved: %d\n" ANSI_COLOR_RESET, bufflen );

	// reading what server sent back
	char* bufferbytes = (char*)malloc( bufflen + 1 );
	bufferbytes[0] = '\0';
	rdres = 1;
	i = 0;
	while( rdres > 0 ){
		rdres = read( sd, bufferbytes+i, bufflen-i );

		//printf("bufflen - i : %d\n", bufflen-i);
		//printf( "number of bytes read: %d\n", rdres );
		if( rdres == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       	return -1;
		}
		i += rdres;
	}
		
	bufferbytes[bufflen] = '\0';
	//printf( ANSI_COLOR_MAGENTA "Buffer received: %s\n" ANSI_COLOR_RESET, bufferbytes );
	if(strcmp(bufferbytes, 	"NO") == 0){
		free(bufferbytes);
		return -1;
	}else{
		parseProtoc(&bufferbytes, 0);				
		printf( ANSI_COLOR_MAGENTA "%s\n" ANSI_COLOR_RESET, bufferbytes );
		free(bufferbytes);

		//now sys call to output to stdout
		char * cmd = (char*)malloc(sizeof(char)*12);
		strcpy(cmd, "cat history");
		if((system(cmd))==-1){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                     	free(cmd);
			return -1;
		}

		free(cmd);	
	}

	return 0;
}

int writeToSocket( char **, int);
int searchProject(char *);

int searchProject(char * proj){
	DIR * dirp;
	char * path = (char *)malloc(2 + strlen(proj) + 1);
	
	path[0]='\0';
	strcat(path, "./");
	strcat(path, proj);
	//printf("PATHSearch: %s\n", path);
	if((dirp = opendir(path)) == NULL){
		free(path);
		return -5;
	}	
	free(path);
	return 0;

}

int writeToSocket(char ** buffer, int sd){
	char num[10];
	//char num[10];
	sprintf( num, "%010d", strlen(*buffer) );
	//strcat( num, ":\0" );

	//first send bytes "projj"
	int wtres;
	wtres = 1;
	int i = 0;
	while( wtres > 0 ){
		wtres = write( sd, num+i, strlen(num)-i );
		printf("wtres bytes: %d\n", wtres);
		if ( wtres == -1){	
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			return -1;
		}	
		i += wtres;
	}
	printf( ANSI_COLOR_MAGENTA "number of bytes sent: %d\n" ANSI_COLOR_RESET, strlen(*buffer) );
	// send "currverr"
	wtres = 1;
	i = 0;
	while( wtres > 0 ){
		wtres = write( sd, (*buffer)+i, strlen(*buffer)-i );
		if ( wtres == -1){        
                       	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       	return -1;
               	}
		i += wtres;
	}
	printf( ANSI_COLOR_MAGENTA "sent2server: %s\n" ANSI_COLOR_RESET, *buffer );
}




int currentversion(char ** projname, int sd, int cm){
	char * cmd = (char*)malloc(sizeof(char)*15);
//	buffer[0]='\0';
	strcpy(cmd, "currentversion");
	
	//sprintf( num, "%010d", strlen(buffer) );
	//strcat( num, ":\0" );

	//first send bytes "create"
	writeToSocket(&cmd, sd);
	free(cmd);


	char *pname = (char*)malloc(sizeof(char)* (strlen(*projname)+1));
	pname[0]='\0';
	strcpy(pname, *projname);

	writeToSocket(&pname, sd);

	free(pname);
	//now see what server sends back
	// reading the number of bytes in the buffer sent back
	char buf2[10];
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){	
			rdres = read( sd, buf2+i, 10-i );
			printf( "number of bytes read: %d\n", rdres );
			if( rdres == -1 ){	
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        	close(sd); return -1;
			}
			i += rdres;
	}
	int bufflen = charToInt( (char*)buf2 );
	printf( ANSI_COLOR_MAGENTA "Number of bytes being recieved: %d\n" ANSI_COLOR_RESET, bufflen );
	// reading the actual buffer
	char* bufferbytes = (char*)malloc( bufflen + 1 );
	bufferbytes[0] = '\0';
	rdres = 1;
	i = 0;
	while( rdres > 0 ){
		rdres = read( sd, bufferbytes+i, bufflen-i );

		//printf("bufflen - i : %d\n", bufflen-i);
		//printf( "number of bytes read: %d\n", rdres );
		if( rdres == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       	return -1;
		}
		i += rdres;
	}
		
	bufferbytes[bufflen] = '\0';
	//printf( ANSI_COLOR_MAGENTA "Buffer received: %s\n" ANSI_COLOR_RESET, bufferbytes );
	if(strcmp(bufferbytes, 	"Error") == 0){
		free(bufferbytes);
		return -1;
	}else{
		parseProtoc(&bufferbytes, cm);			
		if(cm == 0){
			char * cmd = (char*)malloc(sizeof(char)*13);
			strcpy(cmd, "cat ./.s_man");

			if(system(cmd)==-1){
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       		return -1;
			}	
		}	
		free(bufferbytes);
	}

	return 0;


}


int create (char ** projname, int sd){
	char num[10];
	char * buffer = (char*)malloc(sizeof(char)*7);
//	buffer[0]='\0';
	strcpy(buffer, "create");
	
	//sprintf( num, "%010d", strlen(buffer) );
	//strcat( num, ":\0" );

	//first send bytes "create"
	writeToSocket(&buffer, sd);
	free(buffer);

	// first send the number of projname bytes	
	sprintf( num, "%010d", strlen(*projname) );
	char *pname = (char*)malloc(sizeof(char)* (strlen(*projname)+1));
	
//	pname[0]='\0';
	strcpy(pname, *projname);

	writeToSocket(&pname, sd);

	free(pname);
	

	//now see what server sends back
	// reading the number of bytes in the buffer sent back
	char buf2[10];
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){	
			rdres = read( sd, buf2+i, 10-i );
			printf( "number of bytes read: %d\n", rdres );
			if( rdres == -1 ){	
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        	close(sd); return -1;
			}
			i += rdres;
	}
	int bufflen = charToInt( (char*)buf2 );
	printf( ANSI_COLOR_MAGENTA "Number of bytes being recieved: %d\n" ANSI_COLOR_RESET, bufflen );
	// reading the actual buffer
	char* bufferbytes = (char*)malloc( bufflen + 1 );
	bufferbytes[0] = '\0';
	rdres = 1;
	i = 0;
	while( rdres > 0 ){
		rdres = read( sd, bufferbytes+i, bufflen-i );

		//printf("bufflen - i : %d\n", bufflen-i);
		//printf( "number of bytes read: %d\n", rdres );
		if( rdres == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       	return -1;
		}
		i += rdres;
	}
		
	bufferbytes[bufflen] = '\0';
	printf( ANSI_COLOR_MAGENTA "Buffer received: %s\n" ANSI_COLOR_RESET, bufferbytes );

	if(strcmp(bufferbytes, 	"Error") == 0){
		free(bufferbytes);
		return -1;

	}else if(strcmp(bufferbytes, "Project created in Server\n") == 0){
		int search_res = searchProject(*projname);
		if(search_res == -5){
			char * handshake1 = (char *)malloc(sizeof(char)*3);
		
			handshake1[0]='\0';
			strcpy(handshake1, "OK");
			writeToSocket(&handshake1, sd);

			char buf3[10];
			rdres = 1;
			i = 0;
			while( rdres > 0 ){	
				rdres = read( sd, buf3+i, 10-i );
				printf( "number of bytes read: %d\n", rdres );
				if( rdres == -1 ){	
					printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        		close(sd); return -1;
				}
				i += rdres;
			}
			bufflen = charToInt( (char*)buf3 );
			printf( ANSI_COLOR_MAGENTA "Number of bytes being recieved: %d\n" ANSI_COLOR_RESET, bufflen );
			// reading the actual buffer
			char* bufferbytes1 = (char*)malloc( bufflen + 1 );
			bufferbytes1[0] = '\0';
			rdres = 1;
			i = 0;
			while( rdres > 0 ){
				rdres = read( sd, bufferbytes1+i, bufflen-i );

				//printf("bufflen - i : %d\n", bufflen-i);
				//printf( "number of bytes read: %d\n", rdres );
				if( rdres == -1 ){
					printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       			return -1;
				}
				i += rdres;
			}
		
			bufferbytes1[bufflen] = '\0';
			printf( ANSI_COLOR_MAGENTA "Buffer received: %s\n" ANSI_COLOR_RESET, bufferbytes1 );
		
			parseProtoc(&bufferbytes1, 1);
			free(bufferbytes); free(bufferbytes1); free(handshake1);

		}else{
			char * handshake1 = (char *)malloc(sizeof(char)*3);
			
			handshake1[0]='\0';
			strcpy(handshake1, "NO");
			writeToSocket(&handshake1, sd);

			printf("WARNING: This project already exists in your local repository. A new project with the same name was created on the server. The server will not be sending that project back to you.\n");
			exit(2);
		}

	}

	return 0;
}


int checkout (char **projname, int sd){
	char num[10];
	char * buffer = (char*)malloc(sizeof(char)*9);

	buffer[0]='\0';
	strcpy(buffer, "checkout");
	sprintf( num, "%010d", strlen(buffer) );
	//strcat( num, ":\0" );
	//first send bytes "checkout"
	writeToSocket(&buffer, sd);
	free(buffer);

	// first send the number of projname bytes	
	sprintf( num, "%010d", strlen(*projname) );
	char *pname = (char*)malloc(sizeof(char)* (strlen(*projname)+1));
	
	pname[0]='\0';
	strcpy(pname, *projname);
	writeToSocket(&pname, sd);
	free(pname);
	
	// reading the number of bytes in the buffer sent back
	char buf2[10];
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){	
			rdres = read( sd, buf2+i, 10-i );
			printf( "number of bytes read: %d\n", rdres );
			if( rdres == -1 ){	
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        	close(sd); return -1;
			}
			i += rdres;
	}
	int bufflen = charToInt( (char*)buf2 );
	printf( ANSI_COLOR_MAGENTA "Number of bytes being recieved: %d\n" ANSI_COLOR_RESET, bufflen );
	// reading the actual buffer
	char* bufferbytes = (char*)malloc( bufflen + 1 );
	bufferbytes[0] = '\0';
	rdres = 1;
	i = 0;
	while( rdres > 0 ){
		rdres = read( sd, bufferbytes+i, bufflen-i );

		//printf("bufflen - i : %d\n", bufflen-i);
		//printf( "number of bytes read: %d\n", rdres );
		if( rdres == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       	return -1;
		}
		i += rdres;
	}
		
	bufferbytes[bufflen] = '\0';
	printf( ANSI_COLOR_MAGENTA "Buffer received: %s\n" ANSI_COLOR_RESET, bufferbytes );
	//call parsebuff here		
	parseProtoc(&bufferbytes, 1);
	free(bufferbytes);
	return 0;
}


int charToInt( char* numArr ){
        // used to decipher how many bytes are being sent so string issues stop arising
        // number will end with ":" to tell the user that the number ended
        int i;
        for( i = 0; numArr[i] != ':'; i++ );// this is number of digits in the number; 
	char* num = (char*)malloc( i + 1 );
        int j;
        for( j = 0; j < i; j++ )
        	num[j] = numArr[j];
        num[i] = '\0';
        int bytes = atoi(num);
        free( num );
        return bytes;
}

void configure( char* ip, char* port ){
	char* configure_path;
	int len;
	char* strbuff;
	int fd;

	configure_path = (char*)malloc(13*sizeof(char));
	configure_path[0] = '\0';
	strcpy( configure_path, "./.configure" );
	configure_path[12] = '\0';		
	if( (fd = open( configure_path, O_CREAT | O_TRUNC | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1 ){
        	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
      	}

	len = strlen(ip) + strlen(port) + 1;
	strbuff = (char*)malloc( len + 1 );
	strbuff[0] = '\0';
	strcat( strbuff, ip );
	strcat( strbuff, " " );
	strcat( strbuff, port );
	strbuff[len] = '\0';
	if( write( fd, strbuff, len ) == -1 ){
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        }

	if( close( fd ) == -1 ){
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
	}

	free(configure_path);
	free(strbuff);
}

int main( int argc, char** argv ){
	char* command;
	// check the args
	command = argv[1];	
	if( strcmp(command, "configure") == 0 ){ // create .configure file
		configure( argv[2], argv[3] );  
	}else if( strcmp(argv[1], "add") == 0){
		if( addM( argv[2], argv[3] ) == -1 ){
			printf( "Error: adding file has failed\n" );
		}
	}else if( strcmp(argv[1], "remove") == 0){
		if( removeM( argv[2], argv[3]) == -1){
			printf( "Error: removing file has failed\n" );
		}
	}	
	else{ // temporarily just testing that connecting to server works
	
		char* configure_path;
		int fd;
		struct addrinfo hints;
		struct addrinfo * result;
		int res;
		int sd; 
		char* ip;
		char* port;
		struct stat file_stat;
		char* filebuff;
		int rdres;
		int wtres;
		// call getaddrinfo
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		
		//open .configure to get ip and port
		configure_path = (char*)malloc(13*sizeof(char));
		configure_path[0] = '\0';
                strcpy( configure_path, "./.configure" );
                configure_path[12] = '\0';

		if( stat(configure_path, &file_stat ) == -1 ){
        	        printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
               		exit(2);
       		}
       		if( (fd = open( configure_path, O_RDONLY)) == -1 ){
                	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                	exit(2);
        	}

        	filebuff = (char*)malloc( file_stat.st_size+1 );
		filebuff[0] = '\0';
		rdres = 1;

		//this doesnt have to be in a while loop, we're just reading from an file
        	while( rdres > 0 ){
			rdres = read( fd, filebuff, file_stat.st_size  );
	//		printf( "number of bytes read: %d\n", rdres );
			if( rdres == -1 ){
                		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                		return;
        		}
		}
		//close out the configure file. 
       		if( close(fd) == -1 ){
                	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                	return;
        	}

		ip = strtok( filebuff, " " );
		port = strtok( NULL, " " );
		port[strlen(port)] = '\0';
		printf( "ip: %s port: %s\n", ip, port);	
		// steps to connect to server
		if( (res = getaddrinfo( ip, port, &hints, &result )) != 0 ){
			printf( ANSI_COLOR_CYAN "Error getaddrinfo: %s Line#: %d\n" ANSI_COLOR_RESET, gai_strerror(res), __LINE__ );
			exit(2);
		}   
		
		if( ( sd = socket( result->ai_family, result->ai_socktype , result->ai_protocol )) == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);
		}
		
		//using setsockopt to REUSE port immediately after closing connection
		int enable = 1; 
		if( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        exit(2);
		}

		if( connect( sd, result->ai_addr, result->ai_addrlen ) == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			close(sd);
			exit(2);
		}
		// CONNECTED
		printf("Client connected to server!\n");

		//send message to server
		/* if( strcmp(argv[1], "checkout") == 0){
 *			// call checkout
 *			// must send "checkout" to server as well
 *			// argv[2] should be project name to checkout
 *			// .Manifest will be included in protocol sent over from server / no worries here
 *			// checkout returns -1 if fail, 0 if success
 *			// parseProtoc called after checkout returns.
 *			// int checkout (char * projname);
 *			// checkout sends "checkout" and projname to server	
 * 		}
 *
 * 		*/
	//	char* buffer = argv[1];

		//check for commands here
		if(strcmp(argv[1], "checkout") == 0){
			int retval;
			if( (retval = checkout(&argv[2], sd)) == -1){
				printf("Error. Project checkout failed.\n"); 
				exit(2);
			}
			
			//parseProtoc here or nah just let checkout handle it		
		}else if(strcmp(argv[1], "create") == 0){
 			int retval;
 			if( (retval = create(&argv[2], sd)) == -1){
 				printf("Error. Project creation failed.\n");
 				exit(2);
 			}
   		
 		}else if(strcmp(argv[1], "currentversion") == 0){
			int retval;
			if( (retval = currentversion(&argv[2], sd, 0)) == -1){
				printf("Error. Obtaining current version of project failed.\n");
			}	
		}else if(strcmp(argv[1], "history")==0){
			int retval;
			if( (retval = history(&argv[2], sd)) == -1){
				printf("Error. Failed to obtain project history.\n");
			}
		
		}else if(strcmp(argv[1], "commit") == 0){
			char* cmd = (char*)malloc( 7 );
			cmd[0] = '\0';
			strcat( cmd, "commit" );
			if( writeToSocket( &cmd, sd ) == -1 && writeToSocket(&argv[2], sd) == -1 ){
				printf( "Error: failed to commit project.\n" );
				free( cmd );
			}			
			else{	//command and projname sent
				free( cmd );
				if( commit( argv[2], sd ) == -1 ){
					char* err = "Error";
					writeToSocket( &err, sd );	// send error to the server
					printf( "Error: failed to commit project.\n");
				}
			}
		}
			
		freeaddrinfo( result );
//		free( bufferbytes );
		free( configure_path );
//		free( filebuff );
	}
	return 0;
}
