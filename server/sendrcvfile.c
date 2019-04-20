#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sendrcvfile.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void sendfile( char*, int );
void tarfile( char* );

void sendfile( char* filepath, int sd )
{
	char* buff;
	int bufflen;
	char namelen[10];
	char* filename;
	char filelen[10];
	char* filebuff;
	int fd;
	struct stat file_stat;
	int rdres;
		 
	// 1.Get the filename and bytes
	filename = (char*)(intptr_t)basename( filepath );
	sprintf( namelen, "%d", strlen(filename) );
	// 2. Get the file and bytes
	if( stat( filepath, &file_stat ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
		return;
	}
	sprintf( filelen, "%d", file_stat.st_size );
	if( (fd = open( filepath, O_RDONLY ) ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
	}
	filebuff = (char*)malloc( file_stat.st_size + 1 );
	filebuff[0] = '\0';
	rdres = 1;
	while( rdres > 0 )
	{
		rdres = read( fd, filebuff, file_stat.st_size );
		if( rdres == -1 )
		{
			printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                	return;
		}
	}
	if( close( fd ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
	}
	bufflen = strlen(filebuff);
	filebuff[bufflen] = '\0';
	// 3. Create the buffer to send to client -> protocol = "sendfile:numbytesinname:filename:numbytesinfile:file"
	bufflen = 9 + 1 + strlen(namelen) + 1 + strlen(filename) + 1 + strlen(filelen) + 1 + strlen(filebuff);
	buff = (char*)malloc( bufflen + 1 );
	buff[0] = '\0';
	strcat( buff, "sendfile:" );
	strcat( buff, namelen );
	strcat( buff, ":" );
	strcat( buff, filename );
	strcat( buff, ":" );
	strcat( buff, filelen );
	strcat( buff, ":" );
	strcat( buff, filebuff );
	buff[bufflen] = '\0';
	printf( ANSI_COLOR_YELLOW "buffer = %s\n" ANSI_COLOR_RESET, buff );
	
	// 4. Send the actual file to client
	// 4a. first send the number of bytes that the buffer is
	char len[11];
	sprintf( len, "%d", strlen(buff) );
	strcat( len, ":\0" );
	if( write( sd, len, strlen(len) ) == -1 )
        {
                printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
        }
	printf( ANSI_COLOR_MAGENTA "Number of bytes to read has been sent to server\n" ANSI_COLOR_RESET );
	// 4b. send the file
	if( write( sd, buff, strlen(buff) ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
	}
	printf( ANSI_COLOR_MAGENTA "%s sent to client\n" ANSI_COLOR_RESET, filename );
	free(filebuff);
	free(buff);
}

void tarfile( char* path )
{
	printf( "path sent in tarfile = %s\n length of path = %d\n", path, strlen(path));
	// tar cvf <file>.tgz <filepath>
	char* filename;
	char* command;
	int len;
	filename = (char*)(intptr_t)basename( path );
	printf( "filename = %s\n", filename );
	len = strlen( "tar cvf " ) + strlen( filename ) + 5 + strlen(path);
	command = (char*)malloc( len + 1 );
	command[0] = '\0';
	strcat( command, "tar cvf " );
	strcat( command, filename );
	strcat( command, ".tgz " );
	strcat( command, path );
	command[len] = '\0';

	if( system( command ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %s Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
	}
	printf( ANSI_COLOR_YELLOW "%s.tgz created!\n" ANSI_COLOR_RESET, filename );
	free( command );
	return;
}
/*
int main( int argc, char** argv )
{
	tarfile( "./things.txt" );
	sendfile( "./things.txt", 0 );
	return 0;
}*/
