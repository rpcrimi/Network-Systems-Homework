// Distributed File System
// By: Josh Fermin
// 
// Followed:
// http://www.binarytides.com/server-client-example-c-sockets-linux/

#include <stdio.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdarg.h>
// #include <stdbool.h>

#include "configdfc.h"

#define MAX_BUFFER 2000

int parseConfFile(const char *);
void readUserInput(int);
int connectSocket(int, const char *);
void list(int, char *);
int put(int, char *);
int get(int, char *);
void authenticateUser(int, char *, char *);
int errexit(const char *format, ...);

char username[128];
char password[128];
server * servers; 

// Parses a file you give it
int parseConfFile(const char *filename)
{
	char *line;
    char * token;
    char head[64], tail[256], tail2[256];
    size_t len = 0;
    int read_len = 0;
    int i=0;
    FILE* conf_file = fopen(filename, "r");
    if (conf_file == NULL)
        fprintf(stderr, "Could not open config file.\n");
    servers = malloc(4 * sizeof(server));

    while((read_len = getline(&line, &len, conf_file)) != -1) {
        line[read_len+1] = '\0';
        if (line[0] == '#')
            continue;

        sscanf(line, "%s %s %s", head, tail, tail2);
        if (!strcmp(head, "Server")) {
            token = strtok(tail2, ":");
            strncpy(servers[i].host,token, 20);
            token = strtok(NULL, " ");
            servers[i].port= atoi(token);
            i++;
        } 
        if (!strcmp(head, "Username:")) {
            sscanf(tail, "%s", username);
            // confdfc.username[strlen(confdfc.username)-1] = '\0';
        } 
        if (!strcmp(head, "Password:")) {
            sscanf(tail, "%s", password);
            // confdfc.password[strlen(confdfc.password)-1] = '\0';
        }
    }

    // printf("%d\n", i);
    // printf("%s\n", username);
    // printf("%s\n", servers[0].host);
    fclose(conf_file);

    return i;
}



// Reads user input to get LIST, PUT, GET, QUIT, HELP commands
void readUserInput(int sock) {
    char *line = NULL;      /* Line read from STDIN */
    size_t len;
    ssize_t read;
    char command[8], arg[64];
    int status = 1;
    // int server_reply_len = 0;
    char server_reply[MAX_BUFFER];


    while (status) {
        printf("%s> ", username);
        read = getline(&line, &len, stdin);
        line[read-1] = '\0';

        sscanf(line, "%s %s", command, arg);

        if (!strncasecmp(command, "LIST", 4)) {
            // printf("the command you entered was: %s\n", command);
            // printf("sock num %d\n", sock);
            list(sock, command);
            
        }
        else if (!strncasecmp(command, "GET", 3)) {
            get(sock, command);
        }
        else if (!strncasecmp(command, "PUT", 3)) {

        }
        else if (!strncasecmp(command, "QUIT", 4) || !strncasecmp(command, "EXIT", 4)) {
            status = 0;
        }
        else if (!strncasecmp(command, "HELP", 4)) {
            printf("You can enter the following commands: LIST, GET, PUT, EXIT and QUIT\n");
        }
        else{
            printf("Invalid command. Type \"HELP\" for more options.\n");
        }

        if( recv(sock, server_reply, 2000, 0) < 0)
        {
            errexit("Error in recv: %s\n", strerror(errno));
            // puts("recv failed");
            break;
        }
        // server_reply[len] = '\0';
        puts("Server reply :");
        puts(server_reply);
    }
    printf("Shutting down...\n");
}

void authenticateUser(int sock, char * username, char * password)
{
    char *result = malloc(strlen(username)+strlen(password));//+1 for the zero-terminator
    strcpy(result, "LOGIN:");
    strcat(result, username);
    strcat(result, " ");
    strcat(result, password);
    if (write(sock, result, strlen(result)) < 0){
        // puts("Authentication failed");
        errexit("Error in Authentication: %s\n", strerror(errno));

    }
}

void list(int sock, char * command)
{
    if(write(sock, command, strlen(command)) < 0) {
        // puts("List failed");
        errexit("Error in List: %s\n", strerror(errno));

    }
}

int put(int sock, char *filename)
{

    return 0;
}

int get(int sock, char *filename)
{

    return 0;
}

// Connect to a certain socket
int connectSocket(int port, const char *hostname)
{
    int sock;
    struct sockaddr_in server;
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    // puts("Socket created");
     printf("port num %d\n", port);
    server.sin_addr.s_addr = inet_addr(hostname);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
 
    //Connect to remote server
    if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    authenticateUser(sock, username, password);

    printf("Socket %d connected on port %d\n", sock, ntohs(server.sin_port));

    return sock;
}

int errexit(const char *format, ...) {
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

int main(int argc, char *argv[], char **envp)
{
    if(argv[1])
    {
        int num_servers;
        int i;
        // int server_fd[64];

        // Get the number of servers
        num_servers = parseConfFile(argv[1]);

        // Try to connect to the following servers.
        printf("There are %d servers in the config file.\n", num_servers);
        printf("Attempting to connect...\n\n");
        for (i = 0; i < num_servers; ++i)
        {
            // printf("%d\n", servers[i].port);
            // printf("%s\n", servers[i].host);
            servers[i].fd = connectSocket(servers[i].port, servers[i].host);
        }

        // Try to connect to one of the servers
        for (i = 0; i < num_servers; i++)
        {
            if(servers[i].fd != 1)
            {
                printf("%d\n", servers[i].fd);
                readUserInput(servers[i].fd);
                // connection found, break out of loop.
                break;
            }
        }
    }
    else
    {
        printf("Please specify a config file.\n");
    }
}