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

#include "commitupdate.h"
#include "createprotocol.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int push( char*, int );
int commit( char*, int );
int update( char*, int );
int upgrade( char*, int );
Manifest* createLive( Manifest*, int );
int readFromSocket( char**, int );

int readFromSocket( char** buf, int sd )
{
	char buflen[10];
	if( read(sd, buflen, sizeof(buflen)) == -1){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}

	int len = charToInt((char*)buflen);
	printf("Number of bytes being recieved: %d\n", len);
	
	*buf = (char*)malloc( len + 1 );
	int rdres = 1;
	int i = 0;
	while( rdres > 0 ){
		rdres = read( sd, *buf+i, len-i );
		printf("rdres: %d\n", rdres);
		if( rdres == -1 ){
			printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
			return -1;
		}
		i += rdres;
	}
	(*buf)[len] = '\0';
	printf( "message received from server: %s\n", *buf);	
	return 0;
}

Manifest* createLive( Manifest* client_man, int cmd ) // generates the live hashcodes for each file in manifest
{
	// cmd: 0 is commit, 1 is update 
	Manifest* live;
	Manifest* lptr;
	live = (Manifest*)malloc(sizeof(Manifest));
	live->projversion = client_man->projversion;
	live->filepath = client_man->filepath;
	live->vnum = client_man->vnum;		// increment if hash is different
	live->hash = client_man->hash;	// compute new hash
	live->onServer = client_man->onServer;
	live->removed = client_man->removed;
	live->next = NULL;
	lptr = live;
	Manifest* cptr;
	for( cptr = client_man->next; cptr != NULL; cptr = cptr->next )
	{	
		// only create live nodes for files not removed or that are on the server
		if( cmd == 0 && (strcmp( cptr->removed, "1") == 0 || strcmp(cptr->onServer, "0") == 0) ) continue;
		if( cmd == 1 && strcmp( cptr->removed, "1") == 0 ) continue;
		Manifest* newnode = (Manifest*)malloc(sizeof(Manifest));
		newnode->projversion = cptr->projversion;
		newnode->filepath = cptr->filepath;
		newnode->onServer = cptr->onServer;
		newnode->removed = cptr->removed;
		newnode->hash = hashcode(newnode->filepath);
		if( cmd == 0 && strcmp( newnode->hash, cptr->hash ) != 0 )
		{	// new hash means new version
			int num = atoi(cptr->vnum)+1;
			char buf[11];
			sprintf( buf, "%d", num);
			newnode->vnum = (char*)malloc(strlen(buf) + 1);
			newnode->vnum[0] = '\0';
			strcat( newnode->vnum, buf );
			printf( "newnode->vnum = %s\n", newnode->vnum );	
		}
		else newnode->vnum = cptr->vnum;
		lptr->next = newnode;
		lptr = lptr->next;
	}
	lptr->next = NULL;
	printf( "LIVE HASHES:\n" );	
	printM(live);
	return live;
}

int update( char* projname, int ssd ){
	/*
 *	1. Get the .s_man from the server
 * 	2. Build Manifest for live, client, and server
 * 	3. if server and client manifest are same version, check for uploads
 * 	4. else check for MAD and errors 
 * 	5. if .update is empty output that local project is up to date
 * 	*/
	//check that the proj exists
	char* projpath = searchProj( projname );
	if( projpath == NULL ){
		printf( ANSI_COLOR_RED "Error: project not found in client\n" ANSI_COLOR_RESET );
		return -1;
	}
			
	// fetch server's .Manifest
	// server's .Manifest = .s_manproj<projname>
	// client's .Manifest = .Manifest
	if( currentversion(&projname, ssd, 1) == -1 ) return -1;
	char* sman = (char*)malloc( strlen(projname) + 11 );
	sman[0] = '\0';
	snprintf( sman, strlen(projname)+11, ".s_manproj%s", projname );
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
	free(sman);
	live = createLive( c_man, 1 );
	// check for upload files
	int upload = 0;	// will indicate if there are files to upload
	if( strcmp( s_man->projversion, c_man->projversion ) == 0 )
	{
		// loop through the live manifest and server
		Manifest* lptr;
		for( lptr = live->next; lptr != NULL; lptr = lptr->next )
		{
			if( strcmp( lptr->onServer, "0" ) == 0 )
			{
				printf( ANSI_COLOR_GREEN "U\t%s\n" ANSI_COLOR_RESET, lptr->filepath );
				upload = 1;
				continue;
			}
			// find the file on the server manifest
			Manifest* sptr;
			for( sptr = s_man->next; sptr != NULL && strcmp( sptr->filepath, lptr->filepath ) != 0; sptr = sptr->next );
			if( sptr != NULL )
			{
				if( strcmp( sptr->hash, lptr->hash ) != 0 )
				{
					printf( ANSI_COLOR_GREEN "U\t%s\n" ANSI_COLOR_RESET, lptr->filepath );
					upload = 1;
				}
			}
		}
		if( upload == 1 ){ 
			printf( "Please upload the files listed above.\n" ); 
		}	
		else{
			printf( "There are no updates to be made. The local project is up to date.\n" );
		}		
	} 
	else
	{
		// check for MAD
		// create .Update file
		int ufd;
		char* line;	
		char* uploads = NULL;
		char* updatepath = (char*)malloc( strlen("/.Update") + strlen(projpath)+1 );
		updatepath[0] = '\0';
		strcat( updatepath, projpath );
		strcat( updatepath, "/.Update" );
		updatepath[strlen(updatepath)] = '\0';
		if( (ufd = open( updatepath, O_CREAT | O_TRUNC | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1 )
		{
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
			return -1;
		}
		Manifest* sptr;
		int error = 0;
		for( sptr = s_man->next; sptr != NULL; sptr = sptr->next )
		{	//make sure to check if the file was removed by the client
			int found = 0;
			Manifest* cptr;
			for( cptr = c_man->next; cptr != NULL; cptr = cptr->next )
			{
				if( strcmp( cptr->filepath, sptr->filepath ) == 0 && strcmp( cptr->removed, "0" ) == 0 )
				{
					// need to check the hash of the live version, if the hash is dif from client, ERROR, else M
					found = 1;
					Manifest* lptr;
					for( lptr = live->next; lptr != NULL && strcmp(lptr->filepath, cptr->filepath) != 0; lptr = lptr->next );
					if( strcmp( lptr->vnum, sptr->vnum ) != 0 )
					{
						if( error == 0 && strcmp( lptr->hash, cptr->hash ) == 0 )
						{	// M	filepath\n
							line = (char*)malloc( 2+strlen(sptr->filepath)+2 );
							line[0] = '\0';
							strcat( line, "M\t" );
							strcat( line, sptr->filepath );
							strcat( line, "\n" );
							//write to update file
							if( write( ufd, line, strlen(line) ) == -1 )
							{
								printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
								return -1;
							}	 
							free(line);
						}
						else if( strcmp(lptr->hash, cptr->hash) != 0 )//PRINT ERROR
						{
							printf( ANSI_COLOR_RED "%s\n" ANSI_COLOR_RESET, sptr->filepath );
							error = 1;
						}
					}
					else if( strcmp( lptr->vnum, sptr->vnum ) == 0 && strcmp( lptr->hash, sptr->hash ) != 0 )
					{
						upload = 1;
						if( uploads == NULL ){
							uploads = (char*)malloc( 4 + strlen( lptr->filepath ) );
						}
						else uploads = (char*)realloc( uploads, strlen(uploads)+4+strlen(lptr->filepath));
						strcat( uploads, "U\t" );
						strcat( uploads, lptr->filepath );
						strcat( uploads, "\n" ); 
					}
				} 
			}
			if( found == 0 )	//file is not in the client but is in the server, so A
			{	
				line = (char*)malloc( 2+strlen(sptr->filepath)+2 );
				line[0] = '\0';
				strcat( line, "A\t" );
				strcat( line, sptr->filepath );
				strcat( line, "\n" );
				//write to update file
				if( write( ufd, line, strlen(line) ) == -1 )
				{
					printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
					return -1;
				}	 
				free(line);
			}
		}
		// if no error came up, check for deletes
		// check that each file in the client is in the server, if not, then its a delete
		if( error == 0 )
		{
			Manifest* cptr;
			for( cptr = c_man->next; cptr != NULL; cptr = cptr->next )
			{
				Manifest* sptr;
				for( sptr = s_man; sptr != NULL; sptr = sptr->next )
				{
					if( strcmp( sptr->filepath, cptr->filepath ) == 0 )
						break;
				}
				if( strcmp( cptr->removed, "0" ) == 0 && sptr == NULL )	// filepath not found in server
				{
					line = (char*)malloc( 2+strlen(cptr->filepath)+2 );
					line[0] = '\0';
					strcat( line, "D\t" );
					strcat( line, cptr->filepath );
					strcat( line, "\n" );
					//write to update file
					if( write( ufd, line, strlen(line) ) == -1 )
					{
						printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
						return -1;
					}	 
					free(line);
				}
			}
			if( upload == 1 )
			{
				printf( ANSI_COLOR_GREEN "%sPlease upload the files listed above.\n" ANSI_COLOR_RESET, uploads );
			}
		}
		close(ufd);
		// if an error occurred, delete the .Update file and return -1
		if( error == 1 )
		{	// rm -rf path
			if( remove( updatepath ) == -1 )
			{
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
				return -1;
			} 
			printf( "The files listed above have conflicts. Please resolve these conflicts before updating.\n" );
			return -1;
		}
		// if the .Update file is empty, output 'Up to date' 
		// otherwise, print out the entire .Update file
		struct stat file_stat;
		char* filebuf;
		if( stat( updatepath, &file_stat ) == -1 )
		{	
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
			return -1;
		}
		if( file_stat.st_size == 0 )
			printf( "Project is up to date\n" );
		else
		{
			filebuf = (char*)malloc( file_stat.st_size + 1 );
			if( (ufd = open( updatepath, O_RDONLY )) == -1 )
			{
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
				return -1;
			}
			if( read( ufd, filebuf, file_stat.st_size ) == -1 )
			{	
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
				return -1;
			}
			filebuf[file_stat.st_size] = '\0';
			printf( "%s\n", filebuf );
			free( filebuf );
			close(ufd);
		}
		free(updatepath);
		freeManifest( live );
		freeManifest( c_man );
		freeManifest( s_man );
		printf( ".Update has been created! Please run upgrade to make changes.\n" );
		return 0;
	}
}

int upgrade( char* projname, int ssd )
{
	// send the upgrade command to the server, read back the "okay", write the project name to server, read back whether it exists or not in the server
	char* cmd = "upgrade";
	char* rcv;
	char* end = "end";
	writeToSocket( &cmd, ssd );
	if( readFromSocket( &rcv, ssd ) == -1 ) return -1;	// reads "okay" from server
	free(rcv);
	writeToSocket( &projname, ssd );	// send the project name
	readFromSocket( &rcv, ssd );	// read whether found or not
	if( strcmp( rcv, "projnotfound" ) == 0 )
	{
		free(rcv);
		printf( "Error: project not found by the server\n" );
		return -1;
	}  
	free(rcv);
	// check that the projpath exists
	char* projpath = searchProj( projname );
	if( projpath == NULL ){
		printf( ANSI_COLOR_RED "Error: project not found in client\n" ANSI_COLOR_RESET );
		writeToSocket( &end, ssd );
		return -1; //EXIT NICELY 
	}
	// check that there is a .Update file, and if there is, whether it is empty or not
	char* updatepath;
	struct stat file_stat;
	int sfd;
	int cfd;
	updatepath = (char*)malloc( strlen( projpath ) + strlen("/.Update") + 1);
	updatepath[0] = '\0';
	strcat( updatepath, projpath );
	strcat( updatepath, "/.Update" );
	if( stat(updatepath, &file_stat) == -1 ){
		printf( "Error: no .Update file was found on the client. Please run \"update\" before upgrading.\n" );
		free( updatepath );
		writeToSocket( &end, ssd );
		return -1;
	}else if( file_stat.st_size == 0 ){
		printf( "Project is up to date.\n" );
		if( remove( updatepath ) == -1 ){
			printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
		}
		writeToSocket( &end, ssd );
		return 0;
	}

	// open the .Update file
	int fd;
	char* buf = (char*)malloc( file_stat.st_size + 1 );
	if( ( fd = open( updatepath, O_RDONLY ) ) == -1 ){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
		writeToSocket( &end, ssd );
		return -1;
	}
	if( read( fd, buf, file_stat.st_size ) == -1 ){	
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
		writeToSocket( &end, ssd );
		return -1;
	}
	buf[file_stat.st_size] = '\0';
	// for each line in the .Update file, complete the command
	char* manpath;
	char* filebuf;
	int i = 0;
	printf( ANSI_COLOR_YELLOW ".UPDATE\n%s" ANSI_COLOR_RESET, buf );
	
	while( i<strlen(buf) )
	{	// if ltr is 'D', ignore because .Manifest from server will overwrite
	
		char ltr = buf[i];
		int pathlen;
		printf( "Update command: %c\n", ltr );
		i += 2;	//pointing at beginning of filepath
		int j;
		for( j = i; buf[j] != '\n'; j++ );
		pathlen = j-i; 
		if( ltr == 'A' || ltr == 'M' )	// request the filepath from the server
		{
			char* tmp = (char*)malloc( 1 + pathlen );
			tmp[0] = '\0';
			snprintf( tmp, 1+pathlen, "%s", &buf[i] );
			printf( "requesting %s from the server...\n", tmp );
			writeToSocket( &tmp, ssd );
			readFromSocket( &filebuf, ssd ); // reads the protocol intended to replace	
			parseProtoc( &filebuf, 1 );	// recreate the file 
			printf( "file received!\n" );
			free( filebuf );
			free( tmp );
		}
		i = j + 1;	//pointing at place after \n
	}  
	// get the server's .Manifest to overwrite the client's
	manpath = (char*)malloc( 16 + strlen(projname) );
	manpath[0] = '\0';
	printf( "Getting .Manifest from the server...\n" );
	snprintf( manpath, 16+strlen(projname), "root/%s/.Manifest", projname );
	writeToSocket( &manpath, ssd );
	readFromSocket( &filebuf, ssd );	//read back the .Manifest
	parseProtoc( &filebuf, 1 );		// create the .Manifest
	// delete the .Update file
	if( remove( updatepath ) == -1 ){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
	}
	printf( ".Update has been deleted.\n" );
	writeToSocket( &end, ssd );	// let server know that no more files are requested
	printf( "Project is now up to date!\n" );
	free(manpath);
	free( filebuf );
	free( updatepath );
	free( buf );
	return 0;
}

int commit( char* projname, int ssd )
{	//first check that projpath exists
	char* projpath = searchProj( projname );
	if( projpath == NULL ){
		printf( ANSI_COLOR_RED "Error: project not found in client\n" ANSI_COLOR_RESET );
		return -1; //EXIT NICELY 
	}
	//chek whether .update exists, and if it does that its empty
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

	// send "commit" to the server
	// fetch server's .Manifest
	// server's .Manifest = .s_manproj<projname>
	// client's .Manifest = .Manifest
		
	char* cmd = "commit";
	char* rd;
	char* buf;
	writeToSocket( &cmd, ssd );
	//read "okay" from the server
	if( readFromSocket( &rd, ssd ) == -1 ) return -1;
	//send the project name to server
	writeToSocket( &projname, ssd );
	// read the buffer from server
	readFromSocket( &buf, ssd );
	if( strcmp( buf, "Error: project not found.\n" ) == 0 ){
		printf( "Error: project not found.\n" );
		return -1;
	}
	// send buffer to parseprotocol to create manifest
	parseProtoc( &buf, 1 );	
	// now .s_man exists!
	char* sman = (char*)malloc( strlen(projname)+11);
	sman[0] = '\0';
	snprintf( sman, strlen(projpath)+11, ".s_manproj%s", projname );
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
	free(sman);
	if( strcmp(s_man->projversion, c_man->projversion) != 0 )
	{
		printf( "Error: project is not up to date. Please run update on your project before running commit.\n" );
		// SEND FAIL TO THE SERVER
		freeManifest(s_man);
		freeManifest(c_man);
		free(cman);
		return -1;
	}
	live = createLive( c_man, 0 );

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
	Manifest* lptr;
	for( lptr = live->next; lptr != NULL; lptr = lptr->next ){	
		line = (char*)malloc( 1 + 1 + strlen(lptr->vnum) + 1 + strlen(lptr->filepath) + 1 + strlen(lptr->hash) + 2 );
		line[0] = '\0';	
		Manifest* sptr;	// find the same file in the server manifest
		for( sptr = s_man->next; s_man != NULL; sptr = sptr->next ){
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
			//printf( "lptr->vnum = %s\n", lptr->vnum );
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
			char cmd2[100];
			cmd2[0] = '\0';
			strcat( cmd2, "rm -rf " );
			strcat( cmd2, commitpath );
			if( system( cmd2 ) == -1 ){
				printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
			}	//.commit deleted
			free( commitpath );
			freeManifest(live);
			freeManifest(s_man);
			freeManifest(c_man);
			return -1;
		}
	}
	// check delete and add using the client manifest
	Manifest* cptr;
	for( cptr = c_man->next; cptr != NULL; cptr = cptr->next ){
		line = (char*)malloc( 1 + 1 + strlen(cptr->vnum) + 1 + strlen(cptr->filepath) + 1 + strlen(cptr->hash) + 2 );
		line[0] = '\0';
		if( strcmp(cptr->onServer, "0" ) == 0 && strcmp(cptr->removed, "0") == 0 )	//not on the server so need to add
			strcat( line, "A" );
		else if( strcmp(cptr->removed, "1" ) == 0 && strcmp(cptr->onServer, "1") == 0 ) //client removed
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
	close(cmfd);
	// SEND .COMMIT TO THE SERVER
	// commitProjname
	char* projj = (char*)malloc( 7 + strlen(projname) );
	projj[0] = '\0';
	snprintf( projj, 7+strlen(projname), "commit%s", projname );
	createProtocol( &commitpath, &cmd, &projj, ssd );  
/*	if( system( "rm -rf .s_man" ) == -1 ){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__ );
		return -1;
	}*/
	printf( "Commit successful! .Commit has been sent to the server. Please run \"push\" to save changes to repository.\n" ); 
	freeManifest(live);
	freeManifest(s_man);
	freeManifest(c_man);
	free(commitpath);
	return 0;
}

int push( char* projname, int ssd )
{
	
}
/*
int main( int argc, char** argv )
{
	return 0;
}*/
