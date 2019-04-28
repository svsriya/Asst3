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

void commit( char* );
void update( char* );

void commit( char* projname )
{	//first check that projpath exists
	char* projpath = searchProj( projname );
	if( projpath == NULL ){
		printf( ANSI_COLOR_RED "Error: project not found in client\n" ANSI_COLOR_RESET );
		return; //EXIT NICELY 
	}
	//check whether 
}
