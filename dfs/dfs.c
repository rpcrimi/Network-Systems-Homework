#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <netdb.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <openssl/md5.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/poll.h>

#define LENGTH    32 
#define BUFFERSIZE	4096
extern int errno;

struct configuration{
	char *UN1;
	char *UN2;
	char *PW1;
	char *PW2;
	char rootDIR[50];
};

struct configuration parse_config_file()
{
	char *line, *currentLine;
	char *incoming = calloc(BUFFERSIZE, 1);
	struct configuration validUsers;

	FILE *configFile = fopen("dfs.conf", "r");
	if (configFile != NULL){
        size_t newLen = fread(incoming, 1, BUFFERSIZE, configFile);
        if (newLen == 0)
            fputs("ERROR: Cannot read server config file", stderr);
        fclose(configFile);
    }
    // START READING FILE
    int count = 0;
	int counter = 0;
	while ((line = strsep(&incoming, "\n")) != NULL){
		while ((currentLine = strsep(&line, " ")) != NULL){
			if (count == 0)	
				if (counter == 0)
					validUsers.UN1 = currentLine;
				else
					validUsers.PW1 = currentLine;
			else
				if (counter == 0)
					validUsers.UN2 = currentLine;
				else
					validUsers.PW2 = currentLine;
			counter++;
		}
		count++;
		counter = 0;
	}
	free(incoming);
	return validUsers;
}

void handle_child(int s){
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *incoming_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void GET(int fd, char *root, char *fileName, int fileSize){
	char *file1 = calloc(BUFFERSIZE, 1);
	char *file2 = calloc(BUFFERSIZE, 1);
	char *file3 = calloc(BUFFERSIZE, 1);
	char *file4 = calloc(BUFFERSIZE, 1);
	char *ack = calloc(BUFFERSIZE, 1);
	char *pieces = calloc(BUFFERSIZE, 1);
	char *contents1 = calloc(BUFFERSIZE, 1);
	char *contents2 = calloc(BUFFERSIZE, 1);

	strcpy(pieces, "");

	strcpy(file1, root);
	strcat(file1, "/.");
	strcat(file1, fileName);
	strcat(file1, ".1");
	strcpy(file2, root);
	strcat(file2, "/.");
	strcat(file2, fileName);
	strcat(file2, ".2");
	strcpy(file3, root);
	strcat(file3, "/.");
	strcat(file3, fileName);
	strcat(file3, ".3");
	strcpy(file4, root);
	strcat(file4, "/.");
	strcat(file4, fileName);
	strcat(file4, ".4");
	FILE *fp1 = fopen(file1, "rb");
	FILE *fp2 = fopen(file2, "rb");
	FILE *fp3 = fopen(file3, "rb");
	FILE *fp4 = fopen(file4, "rb");

	if (fp1 != NULL){
		if(fp2 != NULL){
			strcat(pieces, ".1,.2");
			struct stat f1,f2;
			stat(file1, &f1);
			stat(file2, &f2);
			int f1size = f1.st_size;
			int f2size = f2.st_size;
			fread(contents1, 1, f1size, fp1);
			fread(contents2, 1, f2size, fp2);
		}
		else if(fp4 != NULL){
			strcat(pieces, ".4,.1");
			struct stat f4,f1;
			stat(file4, &f4);
			stat(file1, &f1);
			int f4size = f4.st_size;
			int f1size = f1.st_size;
			fread(contents1, 1, f4size, fp4);
			fread(contents2, 1, f1size, fp1);
		}
	}
	else{
		if (fp2 != NULL){
			if(fp3 != NULL){
				strcat(pieces, ".2,.3");
				struct stat f2,f3;
				stat(file2, &f2);
				stat(file3, &f3);
				int f2size = f2.st_size;
				int f3size = f3.st_size;
				fread(contents1, 1, f2size, fp2);
				fread(contents2, 1, f3size, fp3);
			}
		}
		else{
			if (fp3 != NULL){
				if(fp4 != NULL){
					strcat(pieces, ".3,.4");
					struct stat f3,f4;
					stat(file3, &f3);
					stat(file4, &f4);
					int f3size = f3.st_size;
					int f4size = f4.st_size;
					fread(contents1, 1, f3size, fp3);
					fread(contents2, 1, f4size, fp4);
				}
			}		
		}

	}
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	fclose(fp4);
	send(fd, pieces, strlen(pieces), 0);
	read(fd, ack, BUFFERSIZE);	
	if(strstr(ack, "ACK")){
		send(fd, contents1, strlen(contents1), 0);
		strcpy(ack, "");
		read(fd, ack, BUFFERSIZE);
		if(strstr(ack, "ACK")){
			send(fd, contents2, strlen(contents2), 0);
			strcpy(ack, "");
			read(fd, ack, BUFFERSIZE);
			if(strstr(ack, "ACK")){
				printf("GOT YOUR ACK\n");
			}
		}		
	}

	return;
}

void PUT(int fd, char rootDIR[50], char *fileName, int fileSize){
	char *FileRoot = calloc(BUFFERSIZE, 1);
	char *ContentPiece = calloc(BUFFERSIZE, 1);
	int bytes;

	bytes = read(fd, ContentPiece, BUFFERSIZE);
	if (bytes < 0)
		printf("Error reading in file\n");	
	
	memcpy(FileRoot, rootDIR, strlen(rootDIR));
	memcpy(FileRoot+strlen(FileRoot), fileName, strlen(fileName));
	FILE *fp = fopen(FileRoot, "w");
	if (fp == NULL)
		printf("Error opening %s\n", fileName);
	fprintf(fp, "%s", ContentPiece);	

	fclose(fp);
	free(ContentPiece);
	free(FileRoot);
}

void LIST(int fd, char rootDIR[50]){
	char *ack = calloc(BUFFERSIZE, 1);
	char *fileNameSize = calloc(BUFFERSIZE, 1);
	char filesToSend[20][100];
	int index = 0;
	DIR *dir;
	struct dirent *ent;
	if((dir = opendir (rootDIR)) != NULL){
	  while((ent = readdir (dir)) != NULL){
	  	if (!strstr(ent->d_name, ".") || !strstr(ent->d_name, "..")){
	  		if(strlen(ent->d_name) > 2){
	  			strcpy(filesToSend[index], ent->d_name);
	  			index++; 
	  		}

	  	}
	  }
	  closedir (dir);
	} 
	else 
	{
	  perror("");
	  return;
	}
	int i;
	for(i=0;i<=index;i++){
		if(i == index){
			send(fd, "DONE", 4, 0);
			read(fd, ack, 8);
			if(strstr(ack, "ACK DONE")){
				break;
			}
		}
		else{
			char size[10];
			sprintf(size, "%zd", strlen(filesToSend[i]));
			strcpy(fileNameSize, size);
			send(fd, fileNameSize, strlen(fileNameSize), 0);
			read(fd, ack, 8);
			if(strstr(ack, "ACK SIZE")){
				send(fd, filesToSend[i], strlen(filesToSend[i]), 0);
				read(fd, ack, 8);
				if(!strstr(ack, "ACK FILE")){
					printf("Error Sending File = %s\n", filesToSend[i]);
				}				
			}

		}
	}
	return;
}

void handle_request(char * rootDir, char *request, int fd){
	char *requestType = strtok(request, ":");
	char *fileName = strtok(NULL, ":");
	char *fileSize = strtok(NULL, ":");
	send(fd, "ACK", 3, 0);
    if (strcmp(requestType, "GET") == 0)
    {
    	GET(fd, rootDir, fileName, atoi(fileSize));
    }
    else if (strcmp(requestType, "PUT") == 0)
    {
    	PUT(fd, rootDir, fileName, atoi(fileSize));
    } 
    else if (strcmp(requestType, "LIST") == 0)
    {
        LIST(fd, rootDir);
    }
}

int main(int argc, char* argv[]){
	int dfs, new_fd, rv, authorized, pollVAL;  
    int temp = 1;
	struct configuration validUsers;
    struct addrinfo hints, *info, *p;
    struct sockaddr_storage clientAddr; 
    struct sigaction sa;
    char s[INET6_ADDRSTRLEN];
    char rootDIR[50];
    char uBUF[BUFFERSIZE];
    char pBUF[BUFFERSIZE];
    char rBUF[BUFFERSIZE];
    socklen_t clientSIZE;

	validUsers = parse_config_file();
    if(argc < 3)
    {
    	fprintf(stderr, "Need to supply root directory and port number\n");
        exit(1);
    }

    strcpy(validUsers.rootDIR, "");
    strcat(validUsers.rootDIR, argv[1]);
    strcat(validUsers.rootDIR, "/");
    mkdir(validUsers.rootDIR, 0777);
    char *portnum = argv[2];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

	// BIND SOCKET
    if ((rv = getaddrinfo(NULL, portnum, &hints, &info)) != 0)
    {
        fprintf(stderr, "Error getting address information: %s\n", gai_strerror(rv));
        return 1;
    }
	// FIND FIRST AND BIND
    for(p = info; p != NULL; p = p->ai_next) 
    {
        if ((dfs = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("SOCKET ERROR\n");
            continue;
        }

        if (setsockopt(dfs, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) == -1) {
            perror("SOCKET ERROR\n");
            exit(1);
        }

        if (bind(dfs, p->ai_addr, p->ai_addrlen) == -1) {
            close(dfs);
            perror("BIND ERROR\n");
            continue;
        }
        break;
    }
    freeaddrinfo(info);

    if (p == NULL)  
    {
        fprintf(stderr, "\nUnable to bind\n");
        exit(1);
    }

    if (listen(dfs, LENGTH) == -1) 
    {
        perror("ERROR: trying to listen\n");
        exit(1);
    }

    // HANDLE CHILDREN
    sa.sa_handler = handle_child; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
    {
        perror("\nsigaction");
        exit(1);
    }

    printf("\nSuccessfully running on port: %s\n", argv[2]);
	
	// SERVER LOOP
    while(1) 
    {  
        clientSIZE = sizeof clientAddr;
        new_fd = accept(dfs, (struct sockaddr *)&clientAddr, &clientSIZE);
        if (new_fd == -1) 
        {
            perror("ERROR ACCEPTING");
            continue;
        }

        inet_ntop(clientAddr.ss_family, incoming_addr((struct sockaddr *)&clientAddr), s, sizeof s);
        printf("Successfully got connection from %s\n", s);
		
		// CHECK CREDENTIALS
        authorized = 0;
        memset(uBUF, 0, BUFFERSIZE);
        memset(rootDIR, 0, 50);
        strcpy(rootDIR, validUsers.rootDIR);
        read(new_fd, uBUF, BUFFERSIZE);
    	if(strcmp(uBUF, validUsers.UN1) == 0)
    	{
			send(new_fd, "Authorized Username", 19, 0);
			memset(pBUF, 0, BUFFERSIZE);
	        read(new_fd, pBUF, BUFFERSIZE);
	    	if(strcmp(pBUF, validUsers.PW1) == 0)
	    	{
	    		strcat(rootDIR, validUsers.UN1);
	    		mkdir(rootDIR, 0777);	    		
    			send(new_fd, "Authorized Password", 19, 0);
    			authorized = 1;
    		}
	    	else
	    	{
    			send(new_fd, "Invalid Password", 16, 0);
    			close(new_fd);
	    	}
	    }
    	else if(strcmp(uBUF, validUsers.UN2) == 0)
    	{
			send(new_fd, "Authorized Username", 19, 0);
			memset(pBUF, 0, BUFFERSIZE);
	        read(new_fd, pBUF, BUFFERSIZE);
	    	if(strcmp(pBUF, validUsers.PW2) == 0)
	    	{
	    		strcat(rootDIR, validUsers.UN2);	
	    		mkdir(rootDIR, 0777);    		
    			send(new_fd, "Authorized Password", 19, 0);
    			authorized = 1;
    		}
	    	else
	    	{
    			send(new_fd, "Invalid Password", 16, 0);
    			close(new_fd);
	    	}
	    }
    	else
    	{
			send(new_fd, "Invalid Password", 16, 0);
			close(new_fd);
    	}

		//AUTHENTICATED: LISTEN FOR REQUEST
		if (!fork() && authorized)
        { 
            (void) close(dfs);
            while(1)
            {   
                struct pollfd ufds[1];
                ufds[0].fd = new_fd;
                ufds[0].events = POLLIN;
                pollVAL = poll(ufds, 1, 1000);
                if (pollVAL == -1)
                    printf("ERROR: Creating Child\n");   
                else if (pollVAL == 0){
                    printf("Process %d timed out\n", getpid());
                    break;
                }
                // SUCCESS!
                else{
                	memset(rBUF, 0, 20);
            		read(new_fd, rBUF, 20);
            		handle_request(rootDIR, rBUF, new_fd);
                }
            }
            (void) close(new_fd);
            printf("Connection closed\n");
            exit(0);
       }
       (void) close(new_fd);
    }
}