#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include "commands.h" 

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int upgrade( int );
int destory( int );
int deleteDir( char* );
int readFromSock( int, char** );

int upgrade( int csd )
{
	// upgrade command recieved, so send "okay" to the client
	char* success = "okay";
	char* projname;
	char* cmd = "upgrade";
	char* msg;
	writeToSocket( &success, csd );
	readFromSock( csd, &projname );	// get the project name from the client
	if( searchProj( projname ) == -5 ){
		// project not found, send error 
		char* err = "projnotfound";
		writeToSocket( &err, csd );
		return -1;
	}
	// until the message read is "end", keep reading
	readFromSock( csd, &msg );
	while( strcmp( msg, "end" ) != 0 )
	{	// msg = path to a file needed, so send in createProtocol 
		//createProtocol( &msg, &cmd, csd );
		free( msg );
		readFromSock( csd, &msg );  
	}
	// send over the .Manifest
	
	return 0;
}

int readFromSock( int csd, char** buf )
{
	char buflen[11];
        if( read(csd, buflen, sizeof(buflen)) == -1){
                printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }

        int len = charToInt((char*)buflen);
        printf("Number of bytes being recieved: %d\n", len);
	
	char* bufread2 = (char*)malloc( len + 1 );
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){
                rdres = read( csd, bufread2+i, len-i );
                printf("rdres: %d\n", rdres);
                if( rdres == -1 ){
                        printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                        return -1;
                }
                i += rdres;
        }
        bufread2[len] = '\0';
        printf( "received from client: %s\n", bufread2);
	*buf = bufread2;
	return 0; 
}

int deleteDir( char* path )
{
	DIR* dir = opendir(path);
	struct dirent* dp;
	struct stat filestat;

	if( !dir ){	
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	while( (dp = readdir(dir)) != NULL )
	{	// construct the path from readdir
		if( strcmp( dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0 )
		{
			char* tmp = (char*)malloc( strlen(path) + strlen(dp->d_name) + 2);
			tmp[0] = '\0';
			snprintf( tmp, strlen(path)+strlen(dp->d_name)+2, "%s/%s", path, dp->d_name);
			printf( "current path: %s\n", tmp );
			
			if( stat(tmp, &filestat) == -1 ){
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
				return -1;
			}
			else{
				if( S_ISDIR( filestat.st_mode ) == 1 )	//its a directory so call function again and then rmdir
				{
					if( deleteDir( tmp ) == -1 ) return -1;
				}
				else{	//its a file, so delete it
					if( remove( tmp ) == -1 ){
						printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
						return -1;
					}
					printf( "%s is a file that has been deleted\n", tmp );
				}
			}
			free( tmp ); 	
		}
	}
	closedir( dir );
	if( rmdir( path ) == -1 ){	
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	printf( "%s is a directory that has been deleted\n", path );
	return 0;
}

int destroy( int csd )
{	// get the project name
	char* projname;
	char* projpath;	// ./root/projname
	// write to server that command was recieved
	char* send = "recieved";
	writeToSocket( &send, csd );
	if( readFromSock( csd, &projname ) == -1 ){
		char* err = "error";
		writeToSocket( &err, csd );
		return -1;
	}
	if( searchProj( projname ) == -5 )
	{
		// project not found, send error 
		char* err = "projnotfound";
		writeToSocket( &err, csd );
		return -1;
	}
	// LOCK REPOSITORY
	// send the path to the project to traverseDir
	// projpath = root/projname
	projpath = (char*)malloc( 6 + strlen(projname) );
	projpath[0] = '\0';
	strcat( projpath, "root/" );
	strcat( projpath, projname );  
	if( deleteDir( projpath ) == -1 ){
		char* err = "error";
		writeToSocket( &err, csd );
		return -1;
	}
	free(projpath);
	char* msg = "success";
	writeToSocket( &msg, csd );
	return 0;
}
/*
int main( int argc, char** argv )
{
	if( deleteDir( argv[1] ) == -1 )
		printf( "DIR NOT DELETED\n" );
	else
		printf( "DIR DELETED SUCCESSFULLY\n" );
	return 0;
}*/
