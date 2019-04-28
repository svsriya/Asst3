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

int commit( char* );
int update( char* );

int commit( char* projname )
{	//first check that projpath exists
	char* projpath = searchProj( projname );
	if( projpath == NULL ){
		printf( ANSI_COLOR_RED "Error: project not found in client\n" ANSI_COLOR_RESET );
		return; //EXIT NICELY 
	}
	//check whether .update exists, and if it does that its empty
	char* updatepath;
	struct stat file_stat;
	int sfd;
	int cfd;
	updatepath = (char*)malloc( strlen( projpath ) + strlen("/.Update") + 1);
	updatepath[0] = '\0';
	strcat( updatepath, projpath );
	strcat( updatepath, "/.Update" );
	if( stat(updatepath, &file_stat) != -1 && file_stat.st_size > 0 ){
		printf( "Error: a nonempty .Update file was found. Please upgrade your project before running commit.\n" );
		free( updatepath );
		return -1;
	}
	free( updatepath );

	// fetch server's .Manifest
	// I'LL ADD IN ACTUAL STUFF LATER
	// server's .Manifest = .s_man
	// client's .Manifest = .Manifest
	char* sman = "./.s_man";
	char* cman = (char*)malloc( strlen(projpath) + 11 );
	char* sbuff;
	char* cbuff;
	cman[0] = '\0';
	strcat( cman, projpath );
	strcat( cman, "/.Manifest" );
	//get .s_man buffer
	if( stat(sman, &file_stat ) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }
        if( (sfd = open( sman, O_RDONLY)) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }
        sbuff = (char*)malloc( file_stat.st_size+1 );
        if( read( sfd, sbuff, file_stat.st_size  ) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }
	//get .Manifest buffer
	if( stat(cman, &file_stat ) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }
        if( (cfd = open( cman, O_RDONLY)) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }
        cbuff = (char*)malloc( file_stat.st_size+1 );
        if( read( cfd, cbuff, file_stat.st_size  ) == -1 )
        {
                printf( ANSI_COLOR_RED "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
                return -1;
        }
	Manifest* s_man;
	Manifest* c_man;
	Manifest* live;
	build( sbuff, &s_man );
	build( cbuff, &c_man );
	if( strcmp(s_man->projversion, c_man->projversion) != 0 )
	{
		printf( "Error: project is not up to date. Please run update on your project before running commit.\n" );
		// SEND FAIL TO THE SERVER
		return -1;
	}
	return 0;
}

int main( int argc, char** argv )
{
	return 0;
}
