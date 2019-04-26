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
	char* hash;
	int onServer;
	struct manifest* next;
}Manifest;

char* searchProj( char* );
Manifest* build( char* );
void printM( Manifest* );
char* hashcode( char* );
void addM( char*, char* );
void freeManifest( Manifest* );

// returns the  path of the project if it exists, otherwise returns null
char* searchProj( char* projname )
{
        DIR* dirp;
        char* projpath;

        projpath = (char*)malloc( 2 + strlen(projname) + 1 );
        projpath[0] = '\0';
	strcat( projpath, "./" );
        strcat( projpath, projname );

        if( (dirp = opendir(projpath)) == NULL )
                return NULL;
        return projpath;
}

Manifest* build( char* projpath )
{
	char* manpath;
	Manifest* head;
	Manifest* ptr = head;
	struct stat file_stat;
	int fd;
	char* mbuffer;
	char* line;
	// 1. open the .Manifest file for the project
	// need to build path for .Manifest
	manpath = (char*)malloc( strlen(projpath) + strlen("/.Manifest") + 1 );
	manpath[0] = '\0';
	strcat( manpath, projpath );
	strcat( manpath, "/.Manifest" );
	if( stat( manpath, &file_stat ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
	}
	mbuffer = (char*)malloc( file_stat.st_size + 1 );
	if( (fd = open( manpath, O_RDONLY )) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
	}
	if( read( fd, mbuffer, file_stat.st_size ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return;
	}
	// 2. skip first line of the manifest and start adding to linked list
	int i;
	for( i = 0; mbuffer[i] != '\n'; i++ );
	i++;	// pointing to first entry
	while( i < strlen(mbuffer) )
	{
		ptr = (Manifest*)malloc( sizeof(Manifest) );
		int len;
		// gets entire line 
		for( len = 0; mbuffer[len+i] != '\n'; len++ );
		
		
	}
 
	free( mbuffer );
	free( manpath );
	return head;
}

void printM( Manifest* head )
{
	Manifest* ptr = head;
	while( ptr != NULL )
	{
		printf( "%s\t%d\t%d\t%s\n", ptr->filepath, ptr->vnum, ptr->onServer, ptr->hash );
		ptr = ptr->next;		
	}
}

char* hashcode( char* filepath )
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
        
	char* result = (char*)malloc( (strlen(hash)*2) + 1);
	result[0] = '\0';
	int x;
	for( x = 0; x < strlen( hash ); x++ )
		sprintf( result+(x*2), "%02x", hash[x] );
	result[x*2] = '\0'; 
	free(filebuff);
	return result;
} 

void addM( char* projname, char* filename )
{
	//check that project exists on client
	char* projpath = searchProj( projname );
	if( projpath == NULL )
	{
		printf( ANSI_COLOR_RED "Error: project does not locally exist\n" ANSI_COLOR_RESET );
		return;
	}
	printf( ANSI_COLOR_YELLOW "Project locally found!\n" ANSI_COLOR_RESET );
	//check the linked list to see if the entry is already in the manifest
}

void freeManifest( Manifest* head )
{
	Manifest* ptr = head;
	while( ptr != NULL )
	{
		Manifest* prev = ptr;
		ptr = ptr->next;
		free(prev->hash);
		free(prev->filepath);
		free(prev);
	}
}

int main( int argc, char** argv )
{
	char* filepath = argv[1];
	char* hash = hashcode( filepath );
        
        printf( "%s", hash );
        printf( "\n" );
	free(hash);
	//char* result = searchProj( filepath );
	//printf( "res = %s\n", result );
	return 0; 
}
