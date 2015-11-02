// Distributed File System
// By: Josh Fermin
// 
// Followed:
// http://www.binarytides.com/server-client-example-c-sockets-linux/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <dirent.h>

#include "configdfs.h"
#define MAX_BUFFER 2000

user *users;
user currUser;
char server_directory[256] = ".";
int server_port;
int num_users = 0;


void parseConfFile(const char *);
void processRequest(int);
int countLines(const char *);
int listenOnPort(int);
int authenticateUser(int, char *, char *);

void parseConfFile(const char *filename)
{
    char * line = NULL;
    char * token;
    size_t len = 0;
    int read_len = 0;
    int i=0;

    // open config file for reading
    FILE *conf_file = fopen(filename, "r");
    if (conf_file == NULL)
        fprintf(stderr, "Could not open file.");

    // get number of users
    num_users = countLines(filename);
    // printf("%d\n", num_users);
    users = malloc(num_users * sizeof(user));

    // for each line in config, add new user struct
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        token = strtok(line, " ");
        strcpy(users[i].name, token);

        token = strtok(NULL, " ");
        strcpy(users[i].password,token);
        i++;
    }
    fclose(conf_file);
}

int countLines(const char *filename)
{
	FILE *fp = fopen(filename,"r");
	int ch=0;
	int lines=1;

	while(!feof(fp))
	{
		ch = fgetc(fp);
		if(ch == '\n')
		{
		lines++;
		}
	}
	return lines;
}

void processRequest(int socket)
{
    char buf[MAX_BUFFER];
    char arg[64];
    char *token;
    char username[32], passwd[32];
    int read_size    = 0;
    int len = 0;

    while ((read_size = recv(socket, &buf[len], (MAX_BUFFER-len), 0)) > 0)
    { 
        char command[read_size];
        strncpy(command, &buf[len], sizeof(command));
        len += read_size;
        command[read_size] = '\0';

        printf("Found:  %s\n", command);

        // printf("Line: %s\n", command);
        if (strncmp(command, "LOGIN:", 6) == 0)
        {
        	token = strtok(command, ": ");
	        token = strtok(NULL, " ");
            if (token == NULL){
                write(socket, "Invalid Command Format. Please try again.\n", 42);
                close(socket);
                return;
            }
	        strcpy(username, token);
            if (token == NULL){
                write(socket, "Invalid Command Format. Please try again.\n", 42);
                close(socket);
                return;
            }
	        token = strtok(NULL, " ");
	        strcpy(passwd, token);

    	}

        sscanf(command, "%s %s", command, arg);
        // Check Command
        if (strncmp(command, "GET", 3) == 0) {
            printf("GET CALLED!\n");
            // getFile(command, connection);
        } else if(strncmp(command, "LIST", 4) == 0) {
            printf("LIST CALLED:\n");
        	send(socket, command , strlen(command), 0);
            // server_list(username);
        } else if(strncmp(command, "PUT", 3) == 0){
            //Put Call
            printf("PUT Called!\n");

        } else if(strncmp(command, "LOGIN", 4) == 0) {
                        //Check Username and Password
            if (authenticateUser(socket, username, passwd) == 0){
                char *message = "Invalid Username/Password. Please try again.\n";
                write(socket, message, strlen(message));
                close(socket);
                return;
            }
        } else {
            printf("Unsupported Command: %s\n", command);
        }
    }
    if(read_size == 0)
	{
        puts("Client disconnected");
        fflush(stdout);
	}
    else if(read_size == -1)
    {
        perror("recv failed");
    }
     
    return;
}

int authenticateUser(int socket, char * username, char * password) 
{
    int i;
    char directory[4096];

    strcpy(directory, server_directory);
    strcat(directory, username);
    for (i = 0; i < num_users; i++){
        if (strncmp(users[i].name, username, strlen(username)) == 0){
        	if(strncmp(users[i].password, password, strlen(password)) == 0)
        	{
	            currUser = users[i];
	            if(!opendir(directory)) {
	                write(socket, "Directory Doesn't Exist. Creating!\n", 35);
	                mkdir(directory, 0770);
	            }
	            printf("User Authenticated.\n");
	            return 1;
        	}
        }
    }
    return 0;
}




int listenOnPort(int port) 
{
	int socket_desc , client_sock, c, *new_sock;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        if(fork() == 0){
	        printf("Connected! %d\n", server_port);         
	        processRequest(client_sock);    
	        exit(0);    	
        }
         
        puts("Handler assigned");
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[], char **envp)
{
	if(argc != 3)
	{
		printf("Please check your arguments.");
		return -1;
	}

	// create a folder which will hold the contents of the server
    strcat(server_directory, argv[1]);
    strcat(server_directory, "/");
    if(!opendir(server_directory))
    {
    	mkdir(server_directory, 0770);    	
    }

    server_port = atoi(argv[2]);

    parseConfFile("server/dfs.conf");

    listenOnPort(server_port);

    return 1;
}