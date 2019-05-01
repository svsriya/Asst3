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

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

char* searchProj( char* );
int commit( 
// checks whether project already exists or not
char* searchProj( char* projname )
{
	DIR* dirp;
	char* projpath;

	projpath = (char*)malloc( 7 + strlen(projname) + 1 );
	projpath[0] - '\0';
	strcat( projpath, "./root/" );
	strcat( projpath, projname );

	if( (dirp = opendir(projpath)) == NULL )
		return NULL;
	return projpath;
}


int main( int argc, char** argv )
{
	char* result = searchProj( argv[1] );
	printf( "result = %s\n", result );
	return 0;
}
