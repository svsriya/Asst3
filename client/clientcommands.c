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

typedef struct manifest{
	char* filepath;
	int vnum;
	unsigned char* hash;
	int onServer;
	struct manifest* next;
}Manifest;

char* searchProj( char* );
Manifest build( char* );
unsigned char* hashcode( char* );
void addM( char*, char* );

// returns the  path of the project if it exists, otherwise returns null
char* searchProj( char* projname )
{
        DIR* dirp;
        char* projpath;

        projpath = (char*)malloc( 2 + strlen(projname) + 1 );
        strcpy( projpath, "./" );
        strcat( projpath, projname );
        projpath[strlen(projpath)] = '\0';

        if( (dirp = opendir(projpath)) == NULL )
                return NULL;
        return projpath;
}

Manifest build( char* projname )
{
	
}

unsigned char* hashcode( char* filepath )
{
	int fd;
        struct stat file_stat;
        char* filebuff;
	unsigned char* hash;
	
        if( stat( filepath, &file_stat ) == -1 ){
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
        }
        filebuff = (char*)malloc( file_stat.st_size + 1);
        if( (fd = open( filepath, O_RDONLY ) ) == -1 )
        {
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        }
        if( read( fd, filebuff, file_stat.st_size ) == -1 )
        {
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        }
        hash = SHA256( filebuff, strlen(filebuff),0);
        free(filebuff);
	return hash;
} 

void addM( char* projname, char* filename )
{
	//check that project exists on client
	if( searchProj( projname ) == NULL )
	{
		printf( ANSI_COLOR_RED "Error: project does not locally exist\n" );
		return;
	}
	//check the linked list to see if the entry is already in the manifest
}

int main( int argc, char** argv )
{
	char* filepath = argv[1];
	/*unsigned char* hash = hashcode( filepath );
        int i;
        for( i = 0; i < SHA256_DIGEST_LENGTH; i++ )
                printf( "%02x", hash[i] );
        printf( "\n" );*/
	char* result = searchProj( filepath );
	printf( "res = %s\n", result );
	return 0; 
}
