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

int main( int argc, char** argv )
{
	char* filename = "./test.txt";
	int fd;
	struct stat file_stat; 
	char* filebuff;

	if( stat( filename, &file_stat ) == -1 ){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
		return; 
	}	
	filebuff = (char*)malloc( file_stat.st_size + 1);
	if( (fd = open( filename, O_RDONLY ) ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
	}
	if( read( fd, filebuff, file_stat.st_size ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
	}
	unsigned char* hash = SHA256( filebuff, strlen(filebuff),0);

	int i; 
	for( i = 0; i < SHA256_DIGEST_LENGTH; i++ )
		printf( "%02x", hash[i] );
	printf( "\n" );
	return 0;
}
