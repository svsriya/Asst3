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
#include "clientcommands.h"
#include "clientcommands.c"

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
int checkout (char *, int);
int history( char*, int );

int history( char* projname, int sd )
{
	char num[10];
	char* buffer = "history";
	int wtres, rdres;
	sprintf( num, "%010d", strlen(buffer) );
	wtres = 1;
	int i = 0;
	// send the number of bytes in "history" command
	while( wtres > 0 )
	{	
		wtres = write( sd, num+i, strlen(num)-i );
		printf("wtres cmd_bytes: %d\n", wtres);
                if ( wtres == -1){
                        printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        return -1;
                }
                i += wtres;
        }
        printf( ANSI_COLOR_MAGENTA "number of bytes_cmd sent: %d\n" ANSI_COLOR_RESET, strlen(projname) );
	// send the "history" command
	wtres = 1;
        i = 0;
        while( wtres > 0 ){
                wtres = write( sd, buffer+i, strlen(buffer)-i );
                if ( wtres == -1){
                        printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        return -1;
                }
                i += wtres;
        }
        printf( ANSI_COLOR_MAGENTA "cmd sent: %s\n" ANSI_COLOR_RESET, buffer );
	// send the number of bytes in projname 
	sprintf( num, "%010d", strlen( projname ));
	 wtres = 1;
        i = 0;
        while( wtres > 0 ){
                wtres = write( sd, num+i, strlen(num)-i );
                printf("wtres: %d\n", wtres);
                if ( wtres == -1){
                        printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        return -1;
                }
                i += wtres;
        }
        printf( ANSI_COLOR_MAGENTA "number of bytes sent: %d\n" ANSI_COLOR_RESET, strlen(projname) );
	//send the projname 
	wtres = 1;
        i = 0;
        while( wtres > 0 ){
                wtres = write( sd, projname+i, strlen(projname)-i );
                if ( wtres == -1){
                        printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        return -1;
                }
                i += wtres;
        }
        printf( ANSI_COLOR_MAGENTA "data sent: %s\n" ANSI_COLOR_RESET, projname );	
	// reading the number of bytes in the buffer sent back
	char buf2[10];
	rdres = 1;
	i = 0;
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
	//call parsebuff here, should be in ./history		
	parseProtoc(&bufferbytes);
	// now history exists!
	// need to print out history
	struct stat file_stat;
	int hfd;
	char* histbuff;
	char* histpath = "./history";
	if( stat(histpath, &file_stat ) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }
        if( (hfd = open( histpath, O_RDONLY)) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return;
        }
        histbuff = (char*)malloc( file_stat.st_size+1 );
        if( read( hfd, histbuff, file_stat.st_size  ) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return;
        }	//print history
	printf( "%s\n", histbuff );
	free(histbuff);
	free(bufferbytes);
	//remove history
	char* cmd = "rm -rf ./history";
	if( system(cmd) == -1 )
	{
		printf( ANSI_COLOR_RED "Errno: %d Message %s Line %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
	}
	return 0;
}
int checkout (char * projname, int sd){
	char num[10];
	char * buffer = "checkout";
	
	sprintf( num, "%010d", strlen(buffer) );
	//strcat( num, ":\0" );

	//first send "checkout"
	int wtres, rdres;
	wtres = 1;
	int i = 0;
	while( wtres > 0 ){
		wtres = write( sd, num+i, strlen(num)-i );
		printf("wtres cmd_bytes: %d\n", wtres);
		if ( wtres == -1){	
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			return -1;
		}	
		i += wtres;
	}
	printf( ANSI_COLOR_MAGENTA "number of bytes_cmd sent: %d\n" ANSI_COLOR_RESET, strlen(projname) );
	// send "checkout"
	wtres = 1;
	i = 0;
	while( wtres > 0 ){
		wtres = write( sd, buffer+i, strlen(buffer)-i );
		if ( wtres == -1){        
                       	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       	return -1;
               	}
		i += wtres;
	}
	printf( ANSI_COLOR_MAGENTA "cmd sent: %s\n" ANSI_COLOR_RESET, buffer );
	// first send the number of projname bytes	
	sprintf( num, "%010d", strlen(projname) );
	wtres = 1;
	i = 0;
	while( wtres > 0 ){
		wtres = write( sd, num+i, strlen(num)-i );
		printf("wtres: %d\n", wtres);
		if ( wtres == -1){	
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			return -1;
		}	
		i += wtres;
	}
	printf( ANSI_COLOR_MAGENTA "number of bytes sent: %d\n" ANSI_COLOR_RESET, strlen(projname) );
	// send the data
	wtres = 1;
	i = 0;
	while( wtres > 0 ){
		wtres = write( sd, projname+i, strlen(projname)-i );
		if ( wtres == -1){        
                       	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                       	return -1;
               	}
		i += wtres;
	}
	printf( ANSI_COLOR_MAGENTA "data sent: %s\n" ANSI_COLOR_RESET, projname );
	// reading the number of bytes in the buffer sent back
	char buf2[10];
	rdres = 1;
	i = 0;
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
	parseProtoc(&bufferbytes);
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

	configure_path = (char*)malloc(strlen("./client/.configure") + 1 );
	configure_path[0] = '\0';
	strcpy( configure_path, "./client/.configure" );
	configure_path[19] = '\0';		
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
	}
	else if( strcmp( command, "add" ) == 0 ){	// adds the filepath to the .Manifest
		if( addM( argv[2], argv[3] ) == -1 )
		{
			printf( "add has failed. Program will exit\n" );
			exit(2);
		}
	}
	else if( strcmp( command, "remove" ) == 0 ){	// marks the filepath for removal in .Manifest
		if( removeM( argv[2], argv[3] ) == -1 )
		{
			printf( "remove has failed. Program will exit\n" );
			exit(2);
		}
	}
	else{ // temporarily just testing that connecting to server works
		// contains all commands that require connecting to the server
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
		configure_path = (char*)malloc(strlen("./client/.configure")+1);
		configure_path[0] = '\0';
                strcpy( configure_path, "./client/.configure" );
                configure_path[19] = '\0';

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
			if( (retval = checkout(argv[2], sd)) == -1){
				printf("Error. Project checkout failed.\n"); 
				exit(2);
			}
			
			//parseProtoc here or nah just let checkout handle it		
		}
		else if( strcmp( argv[1], "history" ) == 0 ){
			//call history
			if( history(argv[2], sd) == -1 ){
				printf( "Error: history command failed.\n" );
			}
		} 	
		else if( strcmp( argv[1], "update" ) == 0 ){
			// call update
		}
		else if( strcmp( argv[1], "upgrade" ) == 0 ){
                        // call upgrade
                }
		else if( strcmp( argv[1], "commit" ) == 0 ){
			// call commit
		}
		else if( strcmp( argv[1], "push" ) == 0 ){
                        // call push
                }
		else if( strcmp( argv[1], "rollback" ) == 0 ){
			// call rollback
		}
		else if( strcmp( argv[1], "currentversion" ) == 0 ){
                        // call currentversion
                }
		else if( strcmp( argv[1], "create" ) == 0 ){
                        // call create
                }
		else if( strcmp( argv[1], "destroy" ) == 0 ){
                        // call destroy
                }
	
		freeaddrinfo( result );
//		free( bufferbytes );
		free( configure_path );
//		free( filebuff );
	}
	return 0;
}
