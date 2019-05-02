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

int commit( char*, int );
int update( char*, int );
Manifest* createLive( Manifest* );

Manifest* createLive( Manifest* client_man ) // generates the live hashcodes for each file in manifest
{
	Manifest* live;
	Manifest* lptr;
	live = (Manifest*)malloc(sizeof(Manifest));
	live->projversion = client_man->projversion;
	live->filepath = client_man->filepath;
	live->vnum = client_man->vnum;		// increment if hash is different
	live->hash = client_man->filepath;	// compute new hash
	live->onServer = client_man->onServer;
	live->removed = client_man->removed;
	lptr = live;
	Manifest* cptr;
	for( cptr = client_man->next; cptr != NULL; cptr = cptr->next )
	{	// only create live nodes for files not removed or that are on the server
		if( strcmp( cptr->removed, "1") == 0 || strcmp(cptr->onServer, "0") == 0 ) continue;
		Manifest* newnode = (Manifest*)malloc(sizeof(Manifest));
		newnode->projversion = cptr->projversion;
		newnode->filepath = cptr->filepath;
		newnode->onServer = cptr->onServer;
		newnode->removed = cptr->removed;
		newnode->hash = hashcode(newnode->filepath);
		if( strcmp( newnode->hash, cptr->hash ) != 0 )
		{	// new hash means new version
			int num = atoi(cptr->vnum)+1;
			char buf[11];
			sprintf( buf, "%d", num);
			newnode->vnum = buf;
			//printf( "newnode->vnum = %s\n", newnode->vnum );	
		}
		else newnode->vnum = cptr->vnum;
		lptr->next = newnode;
		lptr = lptr->next;
	}
	printf( "LIVE HASHES:\n" );	
	printM(live);
	return live;
}

int update( char* projname, int ssd ){
	//check that the proj exists
	char* projpath = searchProj( projname );
	if( projpath == NULL ){
		printf( ANSI_COLOR_RED "Error: project not found in client\n" ANSI_COLOR_RESET );
		return -1;
	}
	
	return 0;
}

int commit( char* projname, int ssd )
{	//first check that projpath exists
	char* projpath = searchProj( projname );
	if( projpath == NULL ){
		printf( ANSI_COLOR_RED "Error: project not found in client\n" ANSI_COLOR_RESET );
		return -1; //EXIT NICELY 
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
	cman[0] = '\0';
	strcat( cman, projpath );
	strcat( cman, "/.Manifest" );
	
	// create the linked lists for manifests and live 
	Manifest* s_man;
	Manifest* c_man;
	Manifest* live;
	build( sman, &s_man );
	build( cman, &c_man );
	free(cman);
	if( strcmp(s_man->projversion, c_man->projversion) != 0 )
	{
		printf( "Error: project is not up to date. Please run update on your project before running commit.\n" );
		// SEND FAIL TO THE SERVER
		freeManifest(s_man);
		freeManifest(c_man);
		free(cman);
		return -1;
	}
	live = createLive( c_man );
	printf( "live->vnum = %s\n", live->next->vnum );
	// WRITE .COMMIT
	// <code><\t><live file version><\t><path><\t><live hash><\n>
	int cmfd;
	char* line;
	char* commitpath = (char*)malloc( strlen("/.Commit") + strlen(projpath)+1 );
	commitpath[0] = '\0';
	strcat( commitpath, projpath );
	strcat( commitpath, "/.Commit" );
	commitpath[strlen(commitpath)] = '\0';
	if( (cmfd = open( commitpath, O_CREAT | O_TRUNC | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1 )
        {
                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
        	return -1;
	}
	// check modify by looking at all the nodes in live and compare with server....n^2 time?
	printf( "before for loop live->vnum = %s\n", live->next->vnum );
	Manifest* lptr;
	for( lptr = live->next; lptr != NULL; lptr = lptr->next ){	
		printf( "before compare lptr->vnum = %s\n", lptr->vnum );
		line = (char*)malloc( 1 + 1 + strlen(lptr->vnum) + 1 + strlen(lptr->filepath) + 1 + strlen(lptr->hash) + 2 );
		line[0] = '\0';	
		Manifest* sptr;	// find the same file in the server manifest
		for( sptr = s_man->next; s_man != NULL; s_man = s_man->next ){
			if( strcmp( sptr->filepath, lptr->filepath ) == 0 ) break;
		}
		if( strcmp( lptr->vnum, sptr->vnum ) > 0 ){
			strcat( line, "M" );	
			strcat( line, "\t" );
			strcat( line, lptr->vnum );
			strcat( line, "\t" );
			strcat( line, lptr->filepath );
			strcat( line, "\t" );
			strcat( line, lptr->hash );
			strcat( line, "\n" );
			printf( "lptr->vnum = %s\n", lptr->vnum );
			if( write( cmfd, line, strlen(line) ) == -1 )
                	{
	                	printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                		return -1;
			}	 
			free(line);
		}
		else if( strcmp( lptr->vnum, sptr->vnum) == 0 ){	// do nothing
			free(line);
			continue;
		}
		else{ //commit failed
			free(line);
			printf( ANSI_COLOR_RED "Error: client must synch with the respository before commiting changes. Please run update.\n" ANSI_COLOR_RESET );
			char cmd[100];
			cmd[0] = '\0';
			strcat( cmd, "rm -rf " );
			strcat( cmd, commitpath );
			system( cmd );	//.commit deleted
			free( commitpath );
			freeManifest(live);
			freeManifest(s_man);
			freeManifest(c_man);
			// WRITE FAIL TO SERVER	
			return -1;
		}
	}
	// check delete and add using the client manifest
	Manifest* cptr;
	for( cptr = c_man->next; cptr != NULL; cptr = cptr->next ){
		line = (char*)malloc( 1 + 1 + strlen(cptr->vnum) + 1 + strlen(cptr->filepath) + 1 + strlen(cptr->hash) + 2 );
		line[0] = '\0';
		if( strcmp(cptr->onServer, "0" ) == 0 )	//not on the server so need to add
			strcat( line, "A" );
		else if( strcmp(cptr->removed, "1" ) == 0 ) //client removed
			strcat( line, "D" );
		else{
			free(line);
			continue;
		}
		strcat( line, "\t" );
		strcat( line, cptr->vnum );
		strcat( line, "\t" );
		strcat( line, cptr->filepath );
		strcat( line, "\t" );
		strcat( line, cptr->hash );
		strcat( line, "\n" );
		if( write( cmfd, line, strlen(line) ) == -1 )
                {
	                printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
                	return -1;
		}	 
		free(line);
	}
	// SEND .COMMIT TO THE SERVER
	// system( "rm -rf .s_man" );
	printf( "commit successful! .Commit has been sent to the server. Please run \"push\" to save changes to repository.\n" ); 
	freeManifest(live);
	freeManifest(s_man);
	freeManifest(c_man);
	free(commitpath);
	return 0;
}

int main( int argc, char** argv )
{
	int res = commit( argv[1] );
	return 0;
}
