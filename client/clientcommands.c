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
#include "clientcommands.h"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct manifest{
	char* projversion;
	char* filepath;
	char* vnum;
	char* hash;
	char* onServer;
	char* removed;
	struct manifest* next;
}Manifest;

int destroy( char*, int );
char* searchProj( char* );
void build( char*, Manifest** );
void writeM( char*, Manifest* );
void printM( Manifest* );
char* hashcode( char* );
int addM( char*, char* );
int removeM( char*, char* );
void freeManifest( Manifest* );

int destroy( char* projname, int ssd )
{
	//send destroy command to server + projname
	// read whether it was a succes or not
	char* res1;
	char* res2;
	char* cmd = (char*)malloc( 8 );
	cmd[0] = '\0';
	strcat( cmd, "destroy" );
	writeToSocket( &cmd, ssd );
	free( cmd );
	// read from server that it recieved destroy
	readFromSocket( &res1, ssd );
	if( strcmp( res1, "recieved" ) != 0 ){
		printf( "Error: server was unable to read the command\n" );
		return -1;
	}	
	writeToSocket( &projname, ssd );
	readFromSocket( &res2, ssd );
	if( strcmp( res2, "projnotfound" ) == 0 ){
		printf( "Error: project not found on server.\n" );
		return -1;
	}
	else if( strcmp( res2, "error" ) == 0 ){
		printf( "Error: unable to destroy project\n" );
		return -1;
	}
	return 0;
}

// returns the  path of the project if it exists, otherwise returns null
char* searchProj( char* projname )
{
        DIR* dirp;
        char* projpath = projname;

        if( (dirp = opendir(projpath)) == NULL )
                return NULL;
        return projpath;
}

void build( char* manpath, Manifest** head )
{	// <filename><\t><v.#><\t><inserver><\t><removed><\t><hash><\n>
	Manifest* ptr = *head;
	struct stat file_stat;
	int fd;
	char* mbuffer;
	char* line;
	// 1. open the .Manifest file for the project
	// need to build path for .Manifest
	printf( "Path of manifest being built: %s\n", manpath );
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
	close(fd);
	mbuffer[file_stat.st_size] = '\0';
	// 2. skip first two lines of the manifest and start adding to linked list
	int i;
	char* projv;
	char* token;
	char s[3] = "\t\n";
	// to avoid weird bugs, copy the result of strtok
	projv = strtok( mbuffer, s );	//proj version
	ptr = (Manifest*)malloc( sizeof(Manifest) );
	
	ptr->projversion = (char*)malloc( strlen(projv) + 1);
	ptr->projversion[0] = '\0';
	strcat( ptr->projversion, projv );

	token = strtok( NULL, s );
	ptr->filepath = (char*)malloc( strlen( token ) + 1 );
        ptr->filepath[0] = '\0';
	strcat(ptr->filepath, token);

	token = strtok( NULL, s );
        ptr->vnum = (char*)malloc( strlen( token ) + 1 );
	ptr->vnum[0] = '\0';
	strcat( ptr->vnum, token );
        
	token = strtok( NULL, s );
	ptr->onServer = (char*)malloc( strlen( token ) + 1 );
	ptr->onServer[0] = '\0';
	strcat( ptr->onServer, token );        

	token = strtok( NULL, s );
	ptr->removed = (char*)malloc( strlen( token ) + 1 );
	ptr->removed[0] = '\0';
	strcat( ptr->removed, token );       
 
	token = strtok( NULL, s );
	ptr->hash = (char*)malloc( strlen( token ) + 1 );
	ptr->hash[0] = '\0';
	strcat( ptr->hash, token );       
 
	token = strtok( NULL, s );
	*head = ptr;
	Manifest* newNode;
	while( token != NULL )
	{
		// first put in project version
		newNode = (Manifest*)malloc( sizeof(Manifest) );
		newNode->projversion = (char*)malloc( strlen( projv ) + 1 );
		newNode->projversion[0] = '\0';
		strcat( newNode->projversion, projv );

		newNode->filepath = (char*)malloc( strlen( token ) + 1 );
		newNode->filepath[0] = '\0';
		strcat( newNode->filepath, token );	
	
		token = strtok( NULL, s );
		newNode->vnum = (char*)malloc( strlen( token ) + 1 );
		newNode->vnum[0] = '\0';
		strcat( newNode->vnum, token );	
	
		token = strtok( NULL, s );
		newNode->onServer = (char*)malloc( strlen( token ) + 1 );
		newNode->onServer[0] = '\0';
		strcat( newNode->onServer, token );		

		token = strtok( NULL, s );
		newNode->removed = (char*)malloc( strlen( token ) + 1 );
		newNode->removed[0] = '\0';
		strcat( newNode->removed, token );

		token = strtok( NULL, s );
		newNode->hash = (char*)malloc( strlen( token ) + 1 );
		newNode->hash[0] = '\0';
		strcat( newNode->hash, token );		

		token = strtok( NULL, s );
		ptr->next = newNode;
		ptr = ptr->next;
	}
	ptr->next = NULL;
	free( mbuffer );
}

void writeM( char* manpath, Manifest* head )
{
	// filepath version server removed hash
	int fd;
	char* buff;
	if( (fd = open( manpath, O_TRUNC | O_APPEND | O_WRONLY ) ) == -1 )
	{
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
		return;	//exit function??
	}
	
	Manifest* ptr;
	for( ptr = head; ptr != NULL; ptr = ptr->next )
	{
		int len = strlen(ptr->filepath)+strlen(ptr->vnum)+strlen(ptr->onServer)+strlen(ptr->removed)+strlen(ptr->hash)+5;
		if( ptr == head ) len += strlen(ptr->projversion)+1;
		buff = (char*)malloc( len + 1 );
		buff[0] = '\0';
		if( ptr == head ){ 
			strcat( buff, ptr->projversion );
			strcat( buff, "\n" );
		}
		strcat( buff, ptr->filepath );
		strcat( buff, "\t" );
		strcat( buff, ptr->vnum );
		strcat( buff, "\t" );
		strcat( buff, ptr->onServer );
		strcat( buff, "\t" );
		strcat( buff, ptr->removed );
		strcat( buff, "\t" );
		strcat( buff, ptr->hash );
		strcat( buff, "\n" );
		if( write( fd, buff, strlen(buff) ) == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
			return; //???
		}	
		free(buff);
	}
	close(fd);
}

void printM( Manifest* head )
{
	Manifest* ptr = head;
	while( ptr != NULL )
	{
		printf( "%s\t%s\t%s\t%s\t%s\n", ptr->filepath, ptr->vnum, ptr->onServer, ptr->removed, ptr->hash );
		ptr = ptr->next;		
	}
}

char* hashcode( char* filepath )
{
	int fd;
        struct stat file_stat;
        char* filebuff;
	unsigned char* hash;
	//printf( "creating hash for: %s\n", filepath );
        if( stat( filepath, &file_stat ) == -1 ){
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                return NULL;
        }
        filebuff = (char*)malloc( file_stat.st_size + 1);
        if( (fd = open( filepath, O_RDONLY ) ) == -1 )
        {
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        	return NULL;
	}
        if( read( fd, filebuff, file_stat.st_size ) == -1 )
        {
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        	return NULL;
	}
	filebuff[file_stat.st_size] = '\0';
        close(fd);
	hash = SHA256( filebuff, strlen(filebuff),0);
	 
	char* result = (char*)malloc( (strlen(hash)*2) + 1);
	result[0] = '\0';
	int x;
	for( x = 0; x < SHA256_DIGEST_LENGTH; x++ )
		sprintf( result+(x*2), "%02x", hash[x] );
	result[x*2] = '\0'; 
	free(filebuff);
	return result;
} 

int addM( char* projname, char* filename )
{
	//check that project exists on client
	char* projpath = searchProj( projname );
	char* manpath;
	Manifest* head = NULL;
	if( projpath == NULL )
	{
		printf( ANSI_COLOR_RED "Error: project does not locally exist\n" ANSI_COLOR_RESET );
		return -1;
	}
	printf( ANSI_COLOR_YELLOW "Project locally found!\n" ANSI_COLOR_RESET );
	manpath = (char*)malloc( strlen(projpath) + strlen("/.Manifest") + 1 );
	manpath[0] = '\0';
	strcat( manpath, projpath );
	strcat( manpath, "/.Manifest" );
	build( manpath, &head );
	//check the linked list to see if the entry is already in the manifest
	Manifest* ptr = head;
	Manifest* prev;
	while( ptr != NULL )
	{
		if( strcmp( ptr->filepath, filename ) == 0 )
		{
			//file already exists so update the hash
			ptr->hash = hashcode(filename);
			if( strcmp(ptr->removed, "1") == 0 ) 
			{
				ptr->removed[0] = '0';
				printf( ANSI_COLOR_YELLOW "File has been added to .Manifest\n" ANSI_COLOR_RESET );
			}
			else printf( ANSI_COLOR_RED "Warning: file already exists in .Manifest, hashcode has been updated\n" ANSI_COLOR_RESET );
			writeM( manpath, head );
		//	printM( head );
			freeManifest( head );
			return 0;
		}  
		prev = ptr;
		ptr = ptr->next;
	}
	// new file!
	ptr = (Manifest*)malloc( sizeof(Manifest) );
	ptr->projversion = (char*)malloc( strlen(prev->projversion) + 1);
	ptr->projversion[0] = '\0';
	strcat( ptr->projversion, prev->projversion );
	ptr->filepath = (char*)malloc( strlen(filename) + 1);
	ptr->filepath[0] = '\0';
	strcat( ptr->filepath, filename );
	ptr->vnum = (char*)malloc( 2 );
	ptr->vnum[0] = '\0';
	strcat( ptr->vnum, "0" );
	ptr->hash = hashcode(filename);
	if( ptr->hash == NULL ) return -1;
	ptr->onServer = (char*)malloc( 2 );
	ptr->onServer[0] = '\0';
	strcat( ptr->onServer, "0" );
	ptr->removed = (char*)malloc( 2 );
	ptr->removed[0] = '\0';
	strcat( ptr->removed, "0" );
	ptr->next = NULL;
	prev->next = ptr;
	writeM( manpath, head );
	//printM( head );
	freeManifest( head );
	free(manpath);
	printf( ANSI_COLOR_YELLOW "File has been added to .Manifest\n" ANSI_COLOR_RESET );
	return 0;
}

int removeM( char* projname, char* filename )
{
	//check that project exists on client
	char* projpath = searchProj( projname );
	char* manpath;
	Manifest* head = NULL;
	if( projpath == NULL )
	{
		printf( ANSI_COLOR_RED "Error: project does not locally exist\n" ANSI_COLOR_RESET );
		return -1;
	}
	printf( ANSI_COLOR_YELLOW "Project locally found!\n" ANSI_COLOR_RESET );
	manpath = (char*)malloc( strlen(projpath) + strlen("/.Manifest") + 1 );
	manpath[0] = '\0';
	strcat( manpath, projpath );
	strcat( manpath, "/.Manifest" );
	build( manpath, &head );
	//check the linked list to see if the entry is already in the manifest
	Manifest* ptr = head;
	while( ptr != NULL )
	{
		if( strcmp( ptr->filepath, filename ) == 0 )
		{
			//check if file is already removed
			if( strcmp( ptr->removed, "1" ) == 0 )
			{
				printf( ANSI_COLOR_RED "Warning: file is already marked as removed in the manifest\n"ANSI_COLOR_RESET );
				return 0;
			} 
			//file already exists so update the hash
			ptr->removed[0] = '1';
			printf( ANSI_COLOR_YELLOW "File has been marked for removal in the manifest\n" ANSI_COLOR_RESET );
			writeM( manpath, head );
		//	printM( head );
			freeManifest( head );
			return 0;
		}  
		ptr = ptr->next;
	}
	printf( ANSI_COLOR_RED "Error: file does not exist in the manifest\n" ANSI_COLOR_RESET );
	freeManifest(head);
	return -1;
}

void freeManifest( Manifest* head )
{
	Manifest* ptr = head;
	while( ptr != NULL )
	{
		Manifest* prev = ptr;
		ptr = ptr->next;
		free(prev->projversion);
		free(prev->filepath);
		free(prev->vnum);
		free(prev->onServer);
		free(prev->removed);
//		free(prev->hash);
		free(prev);
	}
}
/*
int main( int argc, char** argv )
{
	char* projname = argv[2];
	char* filepath = argv[3];
	if( strcmp( argv[1], "add" ) == 0 )
		addM(projname, filepath);
	else if( strcmp( argv[1], "remove" ) == 0 )
		removeM( projname, filepath ); 
	return 0; 
}*/
