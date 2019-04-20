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

int charToInt( char* numArr )
{
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

void configure( char* ip, char* port )
{
	char* configure_path;
	int len;
	char* strbuff;
	int fd;

	configure_path = (char*)malloc(13*sizeof(char));
	configure_path[0] = '\0';
	strcpy( configure_path, "./.configure" );
	configure_path[12] = '\0';		
	if( (fd = open( configure_path, O_CREAT | O_TRUNC | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1 )
	{
        	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
      	}
	len = strlen(ip) + strlen(port) + 1;
	strbuff = (char*)malloc( len + 1 );
	strbuff[0] = '\0';
	strcat( strbuff, ip );
	strcat( strbuff, " " );
	strcat( strbuff, port );
	strbuff[len] = '\0';
	if( write( fd, strbuff, len ) == -1 )
        {
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        }

	if( close( fd ) == -1 )
	{
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
	}

	free(configure_path);
	free(strbuff);
}

int main( int argc, char** argv )
{
	char* command;
	// check the args
	command = argv[1];	
	if( strcmp(command, "configure") == 0 ) // create .configure file
	{
		configure( argv[2], argv[3] );  
	}
	else // temporarily just testing that connecting to server works
	{
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

		if( stat(configure_path, &file_stat ) == -1 )
	        {
        	        printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
               		return;
       		}
       		if( (fd = open( configure_path, O_RDONLY)) == -1 )
       		{
                	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                	return;
        	}
        	filebuff = (char*)malloc( file_stat.st_size+1 );
		filebuff[0] = '\0';
		rdres = 1;
        	while( rdres > 0 )
		{
			rdres = read( fd, filebuff, file_stat.st_size  );
			printf( "number of bytes read: %d\n", rdres );
			if( rdres == -1 )
        		{
                		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                		return;
        		}
		}
       		if( close(fd) == -1 )
        	{
                	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                	return;
        	}
		ip = strtok( filebuff, " " );
		port = strtok( NULL, " " );
		port[strlen(port)-1] = '\0';
		printf( "ip: %s port: %s\n", ip, port);	
		// steps to connect to server
		if( (res = getaddrinfo( ip, port, &hints, &result )) != 0 )
		{
			printf( ANSI_COLOR_CYAN "Error getaddrinfo: %s Line#: %d\n" ANSI_COLOR_RESET, gai_strerror(res), __LINE__ );
			exit(2);
		}   
		
		if( ( sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
		{
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			exit(2);
		}
	
		if( connect( sd, result->ai_addr, result->ai_addrlen ) == -1 )
		{
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			close(sd);
			exit(2);
		}
		// CONNECTED
		printf("Connected to server!\n");

		//send message to server
		char* buffer = argv[1];
		char num[10];
		sprintf( num, "%d", strlen(buffer) );
		strcat( num, ":\0" );
		// first send the number of bytes
		if ( write(sd, num, strlen(num)) == -1){	
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			close(sd); exit(2);
		}	
		printf( ANSI_COLOR_MAGENTA "number of bytes sent: %d\n" ANSI_COLOR_RESET, strlen(buffer) );
		// send the data
		if ( write(sd, buffer, strlen(buffer)) == -1){        
                        printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        close(sd); exit(2);
                }
		printf( ANSI_COLOR_MAGENTA "data sent: %s\n" ANSI_COLOR_RESET, buffer );	
		// reading the number of bytes in the buffer sent back
		char buf2[10];
		rdres = 1;
		while( rdres > 0 )
		{	
			rdres = read( sd, buf2, 10 );
			printf( "number of bytes read: %d\n", rdres );
			if( rdres == -1 )
			{	
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        	close(sd); exit(2);
			}
		}
		int bufflen = charToInt( (char*)buf2 );
		printf( ANSI_COLOR_MAGENTA "Number of bytes being recieved: %d\n" ANSI_COLOR_RESET, bufflen );
		// reading the actual buffer
		char* bufferbytes = (char*)malloc( bufflen + 1 );
		bufferbytes[0] = '\0';
		rdres = 1;
		while( rdres > 0 )
		{
			rdres = read( sd, bufferbytes, bufflen );
			printf( "number of bytes read: %d\n", rdres );
			if( rdres == -1 )
			{
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        	close(sd); exit(2);
			}
		}
		bufferbytes[bufflen] = '\0';
		printf( ANSI_COLOR_MAGENTA "Buffer received: %s\n" ANSI_COLOR_RESET, bufferbytes );
			
		freeaddrinfo( result );
		free( bufferbytes );
		free( configure_path );
		free( filebuff );
	}
	return 0;
}
