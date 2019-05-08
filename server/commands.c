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

int numCommits = 0;

int deleteCmts( char* );
int validCmt( char* );
int upgrade( int );
int destory( int );
int deleteDir( char* );
int readFromSock( int, char** );
int commit( int );
int numDigits( int );
int push( int );
int getProjVersion( char*);
int rollback( int );

int rollback( int csd )
{
	// send okay to the client
	char* projname;
	char* success = "okay";
	writeToSocket( &success, csd );
	readFromSock( csd, &projname );	// get the project name from the client
	if( searchProj( projname ) == -5 ){
		// project not found, send error 
		char* err = "projnotfound";
		writeToSocket( &err, csd );
		return -1;
	}//send that project was found
	writeToSocket( &success, csd );
	
}

int getProjVersion( char* projname )
{
	// open the root/projname/.Manifest
	// read until \n to find project version
	struct stat filestat;
	char* manpath = (char*)malloc( 16+strlen(projname) );
	manpath[0] = '\0';
	snprintf( manpath, 16+strlen(projname), "root/%s/.Manifest", projname );
	int mfd;
	char* filebuf;
	if( stat( manpath, &filestat ) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	if( (mfd = open( manpath, O_RDONLY )) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	filebuf = (char*)malloc( filestat.st_size + 1 );
	if( read( mfd, filebuf, filestat.st_size ) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	} 
	filebuf[filestat.st_size] = '\0';
	close(mfd);
	int numlen;
	for( numlen = 0; filebuf[numlen] != '\n'; numlen++ );
	char num[numlen+1];
	snprintf( num, numlen+1,"%s", filebuf );
	numlen = atoi(num);
	//printf( "Current Project Version: %d\n", numlen );
	free( filebuf );
	free( manpath );
	return numlen;
}

int push( int csd )
{
	// push cmd received, send "okay" to client to acknowledge
	char* success = "okay";
	char* invalid = "invalid";
	char* projname;
	writeToSocket( &success, csd );
	readFromSock( csd, &projname );	// get the project name from the client
	if( searchProj( projname ) == -5 ){
		// project not found, send error 
		char* err = "projnotfound";
		writeToSocket( &err, csd );
		return -1;
	}//send that project was found
	writeToSocket( &success, csd );
	// read back the .Commit sent in
	char* clientbuf;
	readFromSock( csd, &clientbuf );
	parseProtoc( &clientbuf, 1 ); //.Commit reconstructed!
	free( clientbuf );
	// check that it matches one of the ones on file
	// send "invalid" if not
 	char* cmtpath = (char*)malloc( strlen(projname)+14);
	snprintf( cmtpath, strlen(projname)+14, "root/%s/.Commit", projname );
	if( validCmt( cmtpath ) == 1 )	//send okay and delete all commits
	{
		writeToSocket( &success, csd );
		if( deleteCmts( cmtpath ) == -1 )
			return -1;
	}
	else{			// send invalid
		writeToSocket( &invalid, csd );
		return -1;
	}
	// copy the project dir and move into ./prev
	// first check that ./prev exists
	// get the projversion
	DIR* dir = opendir("./prev");
	int projver = getProjVersion( projname );
	char* projdir = (char*)malloc( strlen(projname) + 6 );
	snprintf( projdir, strlen(projname)+6, "root/%s", projname );
	char* prevdir = (char*)malloc( strlen(projname) + numDigits(projver) + 6 );
	snprintf( prevdir, strlen(projname)+numDigits(projver)+6, "prev/%s%d", projname, projver);
	char* command = (char*)malloc(8+strlen(projdir)+strlen(prevdir));
	snprintf( command, 8+strlen(projdir)+strlen(prevdir), "cp -a %s %s", projdir, prevdir );
	//printf( "Current project path: %s\n", projdir );
	//printf( "New project path: %s\n", prevdir );
	if(dir) closedir(dir);
	else mkdir("./prev", S_IRUSR | S_IWUSR | S_IXUSR );
	// copy the dir
	if( system( command ) == -1 ){
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	// read that the client is ready to recieve commands
	readFromSock( csd, &clientbuf );
	free(clientbuf);
	// get the commit's buffer, print to the history, tokenize by line and send to the client
	struct stat cmt1;
	int fd;
	char* filebuf;
	char* cmt = (char*)malloc( strlen(projname)+14);
	snprintf( cmt, strlen(projname)+14, "root/%s/.Commit", projname );
	
	if( stat( cmt, &cmt1 ) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	if( (fd = open( cmt, O_RDONLY )) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	filebuf = (char*)malloc( cmt1.st_size + 1 );
	if( read( fd, filebuf, cmt1.st_size ) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	} 
	filebuf[cmt1.st_size] = '\0';
	close(fd);
	// create history buffer
	char* histpath = (char*)malloc( strlen(projname) + 14 );
	snprintf( histpath, strlen(projname)+14, "root/%s/history", projname );
	char* wrbuf = (char*)malloc( 8+numDigits(getProjVersion(projname))+strlen(filebuf));
	snprintf( wrbuf, 8+numDigits(projver+1)+strlen(filebuf), "\npush\n%d\n%s", projver+1, filebuf);
	
	// tokenize each line and send to client
	char s[2] = "\n";
	char* token = strtok( filebuf, s );
	while( token != NULL )
	{	// token is line, if its A/M get file from client, if its D remove
		char* line = token;
		// send the line to client, read back protocol, parseprotocol
		if( line[0] == 'M' || line[0] == 'A' )
		{
			writeToSocket( &line, csd );	//line
			readFromSock( csd, &clientbuf ); //protoc
			parseProtoc( &clientbuf, csd );	//recreate the file
			free( clientbuf );
		}
		else if( line[0] == 'D' )	// get the filepath to remove
		{
			int i;
			char* delpath;
			for( i = 0; line[i] != '\t'; i++ );
			for( i += 1; line[i] !='\t'; i++ );
			i += 1;	//start of the path
			int j;
			for( j = i; line[j] != '\t'; j++ );
			int len = j-i;
			delpath = (char*)malloc( len+6 );
			snprintf( delpath, len+6, "root/%s", &line[i] );
			printf( "Deleting %s\n", delpath );
			if( remove( delpath ) == -1 )
			{
				printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
				writeToSocket( &invalid, csd );
				return -1;
			}
			free(delpath);
		}
		token = strtok( NULL, s );
	}
	writeToSocket( &success, csd );	//write success ie end to the client
	// get the .Manifest from the client
	readFromSock( csd, &clientbuf );
	parseProtoc( &clientbuf, csd );
	free(clientbuf);
	// send success to client
	writeToSocket( &success, csd );
	// record in the history
	int hfd;
	if( (hfd = open( histpath, O_APPEND | O_WRONLY )) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	if( write( hfd, wrbuf, strlen(wrbuf) ) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	} 
	close( hfd );
	free( histpath );
	free( wrbuf );
	printf( "Respository has been successfully updated\n" );
	return 0;
}

int deleteCmts( char* cmtpath )
{
	// gets the path of where all the commits are 
	DIR* dir = opendir( cmtpath );
	struct dirent* dp;
	printf( "All other commits are being deleted.\n" );
	// open the directory and check each of the .Commits
	if( !dir ){	
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	while( (dp = readdir(dir)) != NULL )
	{	// construct the path from readdir
		if( strcmp( dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0 )
		{
			char* tmp = (char*)malloc( strlen(cmtpath) + strlen(dp->d_name) + 2);
			tmp[0] = '\0';
			snprintf( tmp, strlen(cmtpath)+strlen(dp->d_name)+2, "%s/%s", cmtpath, dp->d_name);
			//printf( "current path: %s\n", tmp );
			// check if its a .Commit file
			char* filename = basename(tmp);
			if( strcmp( filename, ".Commit" ) != 0 && strncmp( filename, ".Commit", 7 ) == 0 )
			{	//delete the .CommitV file 
				if( remove( tmp ) == -1 )
				{
					printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
					return -1;
				}
			}
		}
	}
	closedir(dir);
	printf( "All active commits are deleted\n" );
	return 1;
}

int validCmt( char* cmtpath )
{
	printf( "Verifying that %s is an active commit...\n", cmtpath );
	struct stat cmt1;
	struct stat cmt2;
	int fd1;
	int fd2;
	char* filebuf1;
	char* filebuf2;
	// open the .Commit sent in
	if( stat( cmtpath, &cmt1 ) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	if( (fd1 = open( cmtpath, O_RDONLY )) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	filebuf1 = (char*)malloc( cmt1.st_size + 1 );
	if( read( fd1, filebuf1, cmt1.st_size ) == -1 )
	{
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	} 
	filebuf1[cmt1.st_size] = '\0';
	close(fd1);

	char* cmtdir = dirname( cmtpath );	// gets the path of where all the commits are 
	DIR* dir = opendir( cmtdir );
	struct dirent* dp;
	// open the directory and check each of the .Commits
	if( !dir ){	
		printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		return -1;
	}
	while( (dp = readdir(dir)) != NULL )
	{	// construct the path from readdir
		if( strcmp( dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0 )
		{
			char* tmp = (char*)malloc( strlen(cmtdir) + strlen(dp->d_name) + 2);
			tmp[0] = '\0';
			snprintf( tmp, strlen(cmtdir)+strlen(dp->d_name)+2, "%s/%s", cmtdir, dp->d_name);
			//printf( "current path: %s\n", tmp );
			// check if its a .Commit file
			char* filename = basename(tmp);
			if( strcmp( filename, ".Commit" ) != 0 && strncmp( filename, ".Commit", 7 ) == 0 )
			{	//open .Commit and get filebuf, check if its equal to filebuf1
				if( stat( tmp, &cmt2 ) == -1 )
				{
					printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
					return -1;
				}
				if( (fd2 = open( tmp, O_RDONLY )) == -1 )
				{
					printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
					return -1;
				}
				filebuf2 = (char*)malloc( cmt2.st_size + 1 );
				if( read( fd2, filebuf2, cmt2.st_size ) == -1 )
				{
					printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
					return -1;
				} 
				filebuf2[cmt2.st_size] = '\0';
				close(fd2);
				if( strcmp( filebuf1, filebuf2 ) == 0 )	// .Commit is valid
				{
					printf( "The client has sent a valid commit!\n" );
					free( filebuf1);
					free( filebuf2 );
					return 1;
				}
				free( filebuf2 );
			}
		}
	}
	closedir( dir );	
	free( filebuf1 );
	printf( "The client has send an INVALID commit.\n" );
	return -1; 
}

int numDigits( int num )
{
	int digits = 0;
	if( num == 0 ) return 1;
	while( num != 0 ){
		num = num/10;
		digits++;	
	}
	return digits;
}

int commit( int csd )
{	// commit command recieved, so send "okay" to the client
	char* okay = "okay";
	char* projname;
	char* commit;
	writeToSocket( &okay, csd );
	// read project name from client
	readFromSock( csd, &projname );
	// send the .Manifest to the client
	if( currver( csd, 1, &projname ) == -1 )
		return -1; 
	// read back the .Commit file and recreate
	readFromSock( csd, &commit );
	if( strcmp( commit, "error" ) == 0 )
	{
		printf( "error: failed to commit\n" );
		return -1;
	}
	parseProtoc( &commit, 1 );
	// rename the .Commit file
	char* commitpath;
	char* newcp;
	commitpath = (char*)malloc( 14+strlen(projname) );
	commitpath[0] = '\0';
	snprintf( commitpath, 14+strlen(projname), "root/%s/.Commit", projname );
	newcp = (char*)malloc( 14+strlen(projname)+numDigits(numCommits) );
	newcp[0] = '\0';
	snprintf( newcp, 14+strlen(projname)+numDigits(numCommits), "root/%s/.Commit%d", projname, numCommits );
	if( rename( commitpath, newcp ) == -1 ){
        	printf(ANSI_COLOR_CYAN "Error: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
	}
	printf( "%s has been saved\n", newcp );
	numCommits++;
	return 0;
}

int upgrade( int csd )
{
	// upgrade command recieved, so send "okay" to the client
	char* success = "okay";
	char* projname;
	char* cmd = "upgrade";
	char* msg;
	char* protocolname;
	writeToSocket( &success, csd );
	readFromSock( csd, &projname );	// get the project name from the client
	if( searchProj( projname ) == -5 ){
		// project not found, send error 
		char* err = "projnotfound";
		writeToSocket( &err, csd );
		return -1;
	}//send that project was found
	writeToSocket( &success, csd );
	// until the message read is "end", keep reading
	readFromSock( csd, &msg );
	protocolname = (char*)malloc( 4 + strlen(projname) );
	protocolname[0] = '\0';
	snprintf( protocolname, 4+strlen(projname), "UPG%s", projname );
	while( strncmp( msg, "root/", 5 ) == 0 )
	{	// msg = path to a file needed, so send in createProtocol 
		createProtocol( &msg, &cmd, &protocolname, csd );
		free( msg );
		readFromSock( csd, &msg );  
	}
	free(msg);	
	free(projname);
	free(protocolname);
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
                //printf("rdres: %d\n", rdres);
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
			//printf( "current path: %s\n", tmp );
			
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
