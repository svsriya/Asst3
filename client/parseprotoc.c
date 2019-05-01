#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#include "parseprotoc.h"

void parseProtoc (char **);
void createDir (int, char **);
void createFile (int, char **, int, char **);

void createDir(int subdir_size, char ** subdir_name){

	char * path = malloc(subdir_size +1);
	path[0] = '\0';
	snprintf(path, subdir_size+1, *subdir_name);

	char * path2 = malloc(subdir_size +1);
	path2[0] = '\0';
	snprintf(path2, subdir_size+1, *subdir_name);

	path = path+7;
	path2 = path2+7;

	
	char * dir = dirname(path);
	char * base = basename(path2);

	printf(ANSI_COLOR_CYAN "dir: %s base: %s\n"  ANSI_COLOR_RESET, dir, base);
	
	char * maindir = malloc(sizeof(char)*10000);
	maindir = getcwd(maindir, 10000);
	if(maindir == NULL){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);	
	}

	chdir(dir);	
	mkdir(base, S_IRUSR | S_IWUSR | S_IXUSR);

	chdir(maindir);
	free(maindir); //free(path); free(path2);
	return;
}

void createFile(int filename_size, char ** filename, int filedata_size, char ** filedata){	
	printf("FILENAMEinCF: %s\n", *filename);
	char * path = malloc(filename_size +1);
	path[0] = '\0';
	snprintf(path, filename_size+1, *filename);

	char * path2 = malloc(filename_size +1);
	path2[0] = '\0';
	snprintf(path2, filename_size+1, *filename);
		
	if(strstr("./root", path) != NULL){
		path = path+7;
		path2 = path2+7;
	}
	
	char * dir = dirname(path);
	char * base = basename(path2);


	
	printf(ANSI_COLOR_YELLOW "dir: %s basefile: %s\n"  ANSI_COLOR_RESET, dir, base);
	char * maindir = malloc(sizeof(char)*10000);
	maindir = getcwd(maindir, 10000);

	if(maindir == NULL){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);	
	}

	chdir(dir);

	int newfd;
	//create file
	if( (newfd = open(base, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);	
	}	

//append data to file
	int write_res;
	if((write_res = write(newfd, *filedata, strlen(*filedata))) == -1){
		printf( ANSI_COLOR_CYAN "Errno: %d Message: %s Line#: %d\n" ANSI_COLOR_RESET, errno, strerror(errno), __LINE__);
		exit(2);	
	}

	chdir(maindir);
	free(maindir); 

	//free(path); free(path2);
	printf("done\n");

}
void parseProtoc (char ** buf){
	//tokenize 
	
	char * protoc =  *buf;
	char * tok = strtok (protoc, ":");

	printf(ANSI_COLOR_YELLOW "cmd: %s\n" ANSI_COLOR_RESET, tok);
	if( (strcmp("senddir", tok)) == 0){
		//project folder being sent
		//3 things to look for
		//set first level

		//strtok 3 times

		tok = strtok(NULL, ":"); // -4
		tok = strtok(NULL, ":"); //number of bytes to name of root
		int projdirname_size = atoi(tok);
		
		tok = strtok(NULL, ":"); // name of projdir
		
		//make projdir now
		
		char * base = basename(tok);
		mkdir(base, S_IRUSR | S_IWUSR | S_IXUSR);
		
		while(tok!=NULL){
			//strtok it
			//printf("TOK: %s\n", tok);
			tok = strtok(NULL, ":");
			if(tok == NULL){break;}
		//	tok = strtok(NULL, ":");
			//check for -3 or -4
			if(atoi(tok) == -4){
				tok = strtok(NULL, ":");
				int subdirname_size = atoi(tok); //subdirname_size
				tok = strtok(NULL, ":"); //subdir name

				char * subdirname = malloc(subdirname_size+1); //malloc subdirname
				subdirname[0] = '\0';
				snprintf(subdirname, strlen(tok)+1, tok);

				printf(ANSI_COLOR_CYAN "subdir size: %d subdir_name: %s\n" ANSI_COLOR_RESET, subdirname_size, subdirname);
				
				createDir(subdirname_size, &subdirname);
				free(subdirname);
	
			}else if(atoi(tok) == -3){
				tok = strtok(NULL, ":");
				int filename_size = atoi(tok); //filename_size
				
				tok = strtok(NULL, ":");
				//printf("FILE NAME:  %s\n", tok);
				char * filename = malloc(filename_size +1); //filename (path)
				filename[0] = '\0';
				snprintf(filename, filename_size+1, tok); 
				//printf("FILE NAME:  %s\n", filename);


				tok = strtok(NULL, ":");
				int filedata_size = atoi(tok); //filedata_size

				tok = strtok(NULL, ":");
				char * filedata = malloc(filedata_size+1); //filedata
				filedata[0] = '\0';
				snprintf(filedata, filedata_size+1, tok);

			
				printf(ANSI_COLOR_CYAN "filename_size: %d  file_name: %s fildata_size: %d  file_data: %s\n" ANSI_COLOR_RESET, filename_size, filename, filedata_size, filedata);

				createFile(filename_size, &filename, filedata_size, &filedata);
				free(filename); free(filedata);
			}	

		}
			
	}else if(((strcmp("sendfile", tok)) == 0) || ((strcmp("sendsman", tok)) == 0) ){
		char * cmdd = (char*)malloc(sizeof(char)*9);
		strcpy(cmdd, tok);
		tok = strtok(NULL, ":"); // -3
		tok = strtok(NULL, ":"); //number of bytes name of file
		int filename_size = atoi(tok);
		
		tok = strtok(NULL, ":"); // name of file
		
		char * filename;
		if((strcmp("sendfile", cmdd)) == 0){
			filename = malloc(filename_size +1); //filename (path)
			filename[0] = '\0';
			snprintf(filename, filename_size+1, tok); 
			printf("FILE NAME:  %s\n", filename);
		}else if((strcmp("sendsman", cmdd)) == 0){
			filename = malloc((sizeof(char)*9)); //filename (path)
			filename[0] = '\0';
			//.s_man
			snprintf(filename, 9, "./.s_man"); 
			printf("FILE NAME:  %s\n", filename);
		} 
		free(cmdd);
		tok = strtok(NULL, ":");
		int filedata_size = atoi(tok); //filedata_size

		tok = strtok(NULL, ":");
		char * filedata = malloc(filedata_size+1); //filedata
		filedata[0] = '\0';
		snprintf(filedata, filedata_size+1, tok);
	
		printf(ANSI_COLOR_CYAN "filename_size: %d  file_name: %s fildata_size: %d  file_data: %s\n" ANSI_COLOR_RESET, filename_size, filename, filedata_size, filedata);

		createFile(filename_size, &filename, filedata_size, &filedata);
		free(filename); free(filedata);
	
	}else if((strcmp("Error", tok) == 0)){
		printf("Error: project not found.\n"); exit(2);
	}
}
