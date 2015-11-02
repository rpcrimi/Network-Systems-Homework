#include <errno.h>
#include <time.h>
#include <openssl/md5.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/poll.h>

#define BUFFERSIZE	4096
#ifndef INADDR_NONE
#define INADDR_NONE	0xffffffff
#endif  /* INADDR_NONE */

struct configuration{
	char *myUSERNAME;
	char *myPASSWORD;
	char *DFS1;
	char *DFS2;
	char *DFS3;
	char *DFS4;
};

struct configuration parse_config_file(char* fileName){
	char *line;
	char *source = calloc(BUFFERSIZE, 1);
	struct configuration configFile;
	FILE *myCredentials = fopen(fileName, "r"); //open the config file

	if (myCredentials != NULL){
        size_t newLen = fread(source, 1, BUFFERSIZE, myCredentials);
        if (newLen == 0)
            fputs("Error reading file", stderr);
        fclose(myCredentials);
    }

	int count = 0;
	while ((line = strsep(&source, "\n")) != NULL){
		if (count == 0){	
			configFile.DFS1 = strrchr(line, ':');
			configFile.DFS1++;
		}
		else if (count == 1){	
			configFile.DFS2 = strrchr(line, ':');
			configFile.DFS2++;
		}
		else if (count == 2){	
			configFile.DFS3 = strrchr(line, ':');
			configFile.DFS3++;
		}
		else if (count == 3){	
			configFile.DFS4 = strrchr(line, ':');
			configFile.DFS4++;
		}
		else if (count == 4){	
			configFile.myUSERNAME = strrchr(line, ':');
			configFile.myUSERNAME++;
		}
		else{	
			configFile.myPASSWORD = strrchr(line, ':');
			configFile.myPASSWORD++;
		}
		count++;
	}
	free(source);
	return configFile;
}

int get_hash_val(const char *fileName){
	static char fileContent[BUFFERSIZE];
	unsigned char hash[MD5_DIGEST_LENGTH];
	FILE *fp = fopen(fileName, "rb");
	MD5_CTX mdContext;
	int bytes;
	int hashVal = 0; 

	if (fp != NULL){
		MD5_Init(&mdContext);
	    while((bytes = fread(fileContent, 1, 1024, fp)) != 0)
	        MD5_Update (&mdContext, fileContent, bytes);
	    MD5_Final(hash, &mdContext);
	    hashVal = hash[MD5_DIGEST_LENGTH - 1] % 4;
	    fclose(fp);

	    return hashVal;
	}
	return -1;
}

int connect_socket(const char *host, const char *portnum){
        struct hostent  *hostEnt;
        struct sockaddr_in sin;
        int sd;    

        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

        if ((sin.sin_port=htons((unsigned short)atoi(portnum))) == 0)
        	exit(0);

        if((hostEnt = gethostbyname(host)))
            memcpy(&sin.sin_addr, hostEnt->h_addr, hostEnt->h_length);
        else if((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
        	exit(0);

        sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sd < 0)
        	exit(0);

        if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
            printf("can't connect to %s.%s: %s\n", host, portnum, strerror(errno));
            return -1;
        }

        return sd;
}

char* encrypt_decrypt(char *input, char *key){
	int length = strlen(key);
	int i = 0;
	while(i < strlen(input)){
		input[i] = input[i] ^ key[i % length];
		i++;
	}
	return input;
}

void GET(struct configuration myCredentials, const char *fileName){
	struct stat f;
	char *aBUF = calloc(BUFFERSIZE, 1);
	int hashNUM, dfs1, dfs2, dfs3, dfs4;
	int incompleteFlag = 0;
	char *host = "localhost";

	char *dfs1Contents1 = calloc(BUFFERSIZE, 1);
	char *dfs1Contents2 = calloc(BUFFERSIZE, 1);
	char *dfs2Contents1 = calloc(BUFFERSIZE, 1);
	char *dfs2Contents2 = calloc(BUFFERSIZE, 1);
	char *dfs3Contents1 = calloc(BUFFERSIZE, 1);
	char *dfs3Contents2 = calloc(BUFFERSIZE, 1);
	char *dfs4Contents1 = calloc(BUFFERSIZE, 1);
	char *dfs4Contents2 = calloc(BUFFERSIZE, 1);
	char *pieces = calloc(BUFFERSIZE, 1);

	//Create request
	char *request = calloc(BUFFERSIZE, 1);
	char file[50];
	strcpy(file, fileName);
	stat(file, &f);	
	int fs = f.st_size;
	char fileSize[15];	
	sprintf(fileSize, "%d", fs);

	strcat(request, "GET:");
	strcat(request, file);
	strcat(request, ":");
	strcat(request, fileSize);
	strcat(request, "\0");

	char *ack = calloc(BUFFERSIZE, 1); 
	// Server 1
	dfs1 = connect_socket(host, myCredentials.DFS1);
	if(dfs1 != -1){
		send(dfs1, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);	
		read(dfs1, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			send(dfs1, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
			memset(aBUF, 0, BUFFERSIZE);
			read(dfs1, aBUF, BUFFERSIZE);
			
			if(!strstr(aBUF, "Invalid"))
			{
				send(dfs1, request, strlen(request), 0);
				read(dfs1, ack, BUFFERSIZE);
				if(strstr(ack, "ACK")){
					read(dfs1, pieces, BUFFERSIZE);
					send(dfs1, "ACK", 3, 0);
					read(dfs1, dfs1Contents1, BUFFERSIZE);
					send(dfs1, "ACK", 3, 0);
					read(dfs1, dfs1Contents2, BUFFERSIZE);
					if(strstr(pieces, ".1,.2")){
						hashNUM = 0;
					}
					else if(strstr(pieces, ".4,.1")){
						hashNUM = 1;
					}
					else if(strstr(pieces, ".3,.4")){
						hashNUM = 2;
					}
					else if(strstr(pieces, ".2,.3")){
						hashNUM = 3;
					}
				}
			}
		}		
	}
	close(dfs1);

	// Server 2
	dfs2 = connect_socket(host, myCredentials.DFS2);
	if(dfs2 != -1){
		send(dfs2, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);	
		read(dfs2, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			send(dfs1, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
			memset(aBUF, 0, BUFFERSIZE);
			read(dfs1, aBUF, BUFFERSIZE);
			
			if(!strstr(aBUF, "Invalid"))
			{
				send(dfs2, request, strlen(request), 0);
				read(dfs2, ack, BUFFERSIZE);
				if(strstr(ack, "ACK")){
					read(dfs2, pieces, BUFFERSIZE);
					send(dfs2, "ACK", 3, 0);
					read(dfs2, dfs2Contents1, BUFFERSIZE);
					send(dfs2, "ACK", 3, 0);
					read(dfs2, dfs2Contents2, BUFFERSIZE);

					if(strstr(pieces, ".2,.3")){
						hashNUM = 0;
					}
					else if(strstr(pieces, ".1,.2")){
						hashNUM = 1;
					}
					else if(strstr(pieces, ".4,.1")){
						hashNUM = 2;
					}
					else if(strstr(pieces, ".3,.4")){
						hashNUM = 3;
					}
				}
			}
		}		
	}
	close(dfs2);

	// Server 3
	dfs3 = connect_socket(host, myCredentials.DFS3);
	if(dfs3 != -1){
		send(dfs3, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);	
		read(dfs3, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			send(dfs1, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
			memset(aBUF, 0, BUFFERSIZE);
			read(dfs1, aBUF, BUFFERSIZE);
			
			if(!strstr(aBUF, "Invalid"))
			{
				send(dfs3, request, strlen(request), 0);
				read(dfs3, ack, BUFFERSIZE);
				if(strstr(ack, "ACK")){
					read(dfs3, pieces, BUFFERSIZE);
					send(dfs3, "ACK", 3, 0);
					read(dfs3, dfs3Contents1, BUFFERSIZE);
					send(dfs3, "ACK", 3, 0);
					read(dfs3, dfs3Contents2, BUFFERSIZE);

					if(strstr(pieces, ".3,.4")){
						hashNUM = 0;
					}
					else if(strstr(pieces, ".2,.3")){
						hashNUM = 1;
					}
					else if(strstr(pieces, ".1,.2")){
						hashNUM = 2;
					}
					else if(strstr(pieces, ".4,.1")){
						hashNUM = 3;
					}
				}
			}
		}			
	}
	close(dfs3);

	// Server 4
	dfs4 = connect_socket(host, myCredentials.DFS4);
	if(dfs4 != -1){
		send(dfs4, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);	
		read(dfs4, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			send(dfs1, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
			memset(aBUF, 0, BUFFERSIZE);
			read(dfs1, aBUF, BUFFERSIZE);
			
			if(!strstr(aBUF, "Invalid"))
			{
				send(dfs4, request, strlen(request), 0);
				read(dfs4, ack, BUFFERSIZE);
				if(strstr(ack, "ACK")){
					read(dfs4, pieces, BUFFERSIZE);
					send(dfs4, "ACK", 3, 0);
					read(dfs4, dfs4Contents1, BUFFERSIZE);
					send(dfs4, "ACK", 3, 0);
					read(dfs4, dfs4Contents2, BUFFERSIZE);

					if(strstr(pieces, ".4,.1")){
						hashNUM = 0;
					}
					else if(strstr(pieces, ".3,.4")){
						hashNUM = 1;
					}
					else if(strstr(pieces, ".2,.3")){
						hashNUM = 2;
					}
					else if(strstr(pieces, ".1,.2")){
						hashNUM = 3;
					}
				}
			}
		}			
	}
	close(dfs4);


	char *encrypted = calloc(BUFFERSIZE, 1);
	if(hashNUM == 0){
		if(dfs1 != -1){
			strcpy(encrypted, dfs1Contents1);
			strcat(encrypted, dfs1Contents2);
			if(dfs3 != -1){
				strcat(encrypted, dfs3Contents1);
				strcat(encrypted, dfs3Contents2);
			}
			else if(dfs2 != -1 && dfs4 != -1){
				strcat(encrypted, dfs2Contents2);
				strcat(encrypted, dfs4Contents1);
			}
			else{
				incompleteFlag = 1;
			}

		}
		else if(dfs2 != -1 && dfs4 != -1){
			strcpy(encrypted, dfs4Contents2);
			strcat(encrypted, dfs2Contents1);
			strcat(encrypted, dfs2Contents2);
			strcat(encrypted, dfs4Contents1);
		}
		else{
			incompleteFlag = 1;
		}
	}

	else if(hashNUM == 1){
		if(dfs2 != -1){
			strcpy(encrypted, dfs2Contents1);
			strcat(encrypted, dfs2Contents2);
			if(dfs4 != -1){
				strcat(encrypted, dfs4Contents1);
				strcat(encrypted, dfs4Contents2);
			}
			else if(dfs1 != -1 && dfs3 != -1){
				strcat(encrypted, dfs3Contents2);
				strcat(encrypted, dfs1Contents1);
			}
			else{
				incompleteFlag = 1;
			}

		}
		else if(dfs1 != -1 && dfs3 != -1){
			strcpy(encrypted, dfs1Contents2);
			strcat(encrypted, dfs3Contents1);
			strcat(encrypted, dfs3Contents2);
			strcat(encrypted, dfs1Contents1);
		}
		else{
			incompleteFlag = 1;
		}
	}

	else if(hashNUM == 2){
		if(dfs3 != -1){
			strcpy(encrypted, dfs3Contents1);
			strcat(encrypted, dfs3Contents2);
			if(dfs1 != -1){
				strcat(encrypted, dfs1Contents1);
				strcat(encrypted, dfs1Contents2);
			}
			else if(dfs2 != -1 && dfs4 != -1){
				strcat(encrypted, dfs4Contents2);
				strcat(encrypted, dfs2Contents1);
			}
			else{
				incompleteFlag = 1;
			}

		}
		else if(dfs2 != -1 && dfs4 != -1){
			strcpy(encrypted, dfs2Contents2);
			strcat(encrypted, dfs4Contents1);
			strcat(encrypted, dfs4Contents2);
			strcat(encrypted, dfs2Contents1);
		}
		else{
			incompleteFlag = 1;
		}
	}

	else if(hashNUM == 3){
		if(dfs4 != -1){
			strcpy(encrypted, dfs3Contents1);
			strcat(encrypted, dfs3Contents2);
			if(dfs2 != -1){
				strcat(encrypted, dfs2Contents1);
				strcat(encrypted, dfs2Contents2);
			}
			else if(dfs1 != -1 && dfs3 != -1){
				strcat(encrypted, dfs1Contents2);
				strcat(encrypted, dfs3Contents1);
			}
			else{
				incompleteFlag = 1;
			}

		}
		else if(dfs1 != -1 && dfs3 != -1){
			strcpy(encrypted, dfs3Contents2);
			strcat(encrypted, dfs1Contents1);
			strcat(encrypted, dfs1Contents2);
			strcat(encrypted, dfs3Contents1);
		}
		else{
			incompleteFlag = 1;
		}
	}
	if(!incompleteFlag){
		char *decrypted = encrypt_decrypt(encrypted, myCredentials.myPASSWORD);
		struct stat st;
    	int result = stat(fileName, &st);
    	if(result != 0){
    		remove(fileName);
    	}
		FILE *filep = fopen(fileName, "w");
		if (filep == NULL)
			printf("Error opening %s\n", fileName);
		fprintf(filep, "%s", decrypted);	
		fclose(filep);
		printf("GET: %s success!\n", fileName);			

	}
	else{
		printf("File is incomplete: %s\n", fileName);
	}

	return;
}

void PUT(struct configuration myCredentials, const char *fileName){
	struct stat st;
	char root[50];
	char fileToSend[50];
	char messageSIZE[15];
	char *aBUF = calloc(BUFFERSIZE, 1);
	int pSIZE, hashNUM, lpSIZE; 
	int dfs1, dfs2, dfs3, dfs4;
	char *host = "localhost";
	
	strcpy(root, "./");
	strcat(root, fileName);

	FILE *fp = fopen(root, "rb");
	
	if (fp != NULL){
		hashNUM = get_hash_val(fileName);
	}

	else
	{
		printf("file does not exist: %s\n", fileName);
		return;
	}

	stat(root, &st);
	int BigSize = st.st_size*2;
	char *OrigFile = calloc(BigSize, 1);
	fread(OrigFile, 1, st.st_size, fp);
	OrigFile = encrypt_decrypt(OrigFile, myCredentials.myPASSWORD);
	pSIZE = strlen(OrigFile)/4 + 1;
	lpSIZE = pSIZE + 4;
    
    char *part1 = calloc(pSIZE, 1);
    char *part2 = calloc(pSIZE, 1);
    char *part3 = calloc(pSIZE, 1);
    char *part4 = calloc(lpSIZE, 1);
    
    strncpy(part1, OrigFile, pSIZE);
    strncpy(part2, OrigFile+pSIZE, pSIZE);
    strncpy(part3, OrigFile+(pSIZE*2), pSIZE);
    strncpy(part4, OrigFile+(pSIZE*3), pSIZE);

    fclose(fp);

    int PartSize = strlen(part1);
   	sprintf(messageSIZE, "%d", PartSize);

    strcpy(fileToSend, "/.");
	strcat(fileToSend, fileName);
	strcat(fileToSend, ".1");

	// SERVER 1
	dfs1 = connect_socket(host, myCredentials.DFS1);
	send(dfs1, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);
	read(dfs1, aBUF, BUFFERSIZE);

	if(!strstr(aBUF, "Invalid"))
	{
		send(dfs1, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
		memset(aBUF, 0, BUFFERSIZE);
		read(dfs1, aBUF, BUFFERSIZE);
		
		if(!strstr(aBUF, "Invalid"))
		{
			char *ack = calloc(BUFFERSIZE, 1);
			char *request = calloc(BUFFERSIZE, 1);
			switch(hashNUM)
			{
				case 0 :
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs1, request, 20, 0);
					read(dfs1, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs1, part1, strlen(part1), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '2';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs1, request, 20, 0);
					read(dfs1, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs1, part2, strlen(part2), 0);
					}
					break;

				case 1 :
					fileToSend[strlen(fileToSend)-1] = '4';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs1, request, 20, 0);
					read(dfs1, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs1, part4, strlen(part4), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '1';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs1, request, 20, 0);
					read(dfs1, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs1, part1, strlen(part1), 0);
					}
					break;

				case 2 :
					fileToSend[strlen(fileToSend)-1] = '3';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs1, request, 20, 0);
					read(dfs1, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs1, part3, strlen(part3), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '4';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs1, request, 20, 0);
					read(dfs1, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs1, part4, strlen(part4), 0);
					}
					break;

				case 3 :
					fileToSend[strlen(fileToSend)-1] = '2';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs1, request, 20, 0);
					read(dfs1, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs1, part2, strlen(part2), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '3';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs1, request, 20, 0);
					read(dfs1, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs1, part3, strlen(part3), 0);
					}
					break;
			}
		}
		close(dfs1);
	}
	
	// SERVER 2
	dfs2 = connect_socket(host, myCredentials.DFS2);
	send(dfs2, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);
	memset(aBUF, 0, BUFFERSIZE);
	read(dfs2, aBUF, BUFFERSIZE);

	if(!strstr(aBUF, "Invalid"))
	{
		send(dfs2, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
		
		memset(aBUF, 0, BUFFERSIZE);
		read(dfs2, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			char *ack = calloc(BUFFERSIZE, 1);
			char *request = calloc(BUFFERSIZE, 1);
			switch(hashNUM)
			{
				case 0 :
					fileToSend[strlen(fileToSend)-1] = '2';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part2, strlen(part2), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '3';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part3, strlen(part3), 0);
					}
					break;

				case 1 :
					fileToSend[strlen(fileToSend)-1] = '1';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part1, strlen(part1), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '2';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part2, strlen(part2), 0);
					}
					break;

				case 2 :
					fileToSend[strlen(fileToSend)-1] = '4';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part4, strlen(part4), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '1';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part1, strlen(part1), 0);
					}
					break;

				case 3 :
					fileToSend[strlen(fileToSend)-1] = '3';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part3, strlen(part3), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '4';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part4, strlen(part4), 0);
					}
					break;
			}
		}
		close(dfs2);
	}
	
	// SERVER 3
	dfs3 = connect_socket(host, myCredentials.DFS3);
	send(dfs3, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);
	
	memset(aBUF, 0, BUFFERSIZE);
	read(dfs3, aBUF, BUFFERSIZE);
	if(!strstr(aBUF, "Invalid"))
	{
		send(dfs3, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
		
		memset(aBUF, 0, BUFFERSIZE);
		read(dfs3, aBUF, BUFFERSIZE);
		
		if(!strstr(aBUF, "Invalid"))
		{
			char *ack = calloc(BUFFERSIZE, 1);
			char *request = calloc(BUFFERSIZE, 1);
			switch(hashNUM)
			{
				case 0 :
					fileToSend[strlen(fileToSend)-1] = '3';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part3, strlen(part3), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '4';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part4, strlen(part4), 0);
					}
					break;

				case 1 :
					fileToSend[strlen(fileToSend)-1] = '2';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part2, strlen(part2), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '3';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part3, strlen(part3), 0);
					}
					break;

				case 2 :
					fileToSend[strlen(fileToSend)-1] = '1';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part1, strlen(part1), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '2';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part2, strlen(part2), 0);
					}
					break;

				case 3 :
					fileToSend[strlen(fileToSend)-1] = '4';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part4, strlen(part4), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '1';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part1, strlen(part1), 0);
					}
					break;
			}
		}
		close(dfs3);
	}

	// SERVER 4
	dfs4 = connect_socket(host, myCredentials.DFS4);
	send(dfs4, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);
	
	memset(aBUF, 0, BUFFERSIZE);
	read(dfs4, aBUF, BUFFERSIZE);
	if(!strstr(aBUF, "Invalid"))
	{
		send(dfs4, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
		
		memset(aBUF, 0, BUFFERSIZE);
		read(dfs4, aBUF, BUFFERSIZE);
		
		if(!strstr(aBUF, "Invalid"))
		{
			char *ack = calloc(BUFFERSIZE, 1);
			char *request = calloc(BUFFERSIZE, 1);
			switch(hashNUM)
			{
				case 0 :
					fileToSend[strlen(fileToSend)-1] = '4';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part4, strlen(part4), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '1';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part1, strlen(part1), 0);
					}
					break;

				case 1 :
					fileToSend[strlen(fileToSend)-1] = '3';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part3, strlen(part3), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '4';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part4, strlen(part4), 0);
					}
					break;

				case 2 :
					fileToSend[strlen(fileToSend)-1] = '2';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part2, strlen(part2), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '3';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part3, strlen(part3), 0);
					}
					break;

				case 3 :
					fileToSend[strlen(fileToSend)-1] = '1';
					strcat(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part1, strlen(part2), 0);
					}

					fileToSend[strlen(fileToSend)-1] = '2';
					strcpy(request, "PUT:");
					strcat(request, fileToSend);
					strcat(request, ":");
					strcat(request, messageSIZE);
					strcat(request, "\0");
					send(dfs2, request, 20, 0);
					read(dfs2, ack, BUFFERSIZE);
					if(strstr(ack, "ACK")){
						send(dfs2, part1, strlen(part2), 0);
					}
					break;
			}
		}
		close(dfs4);
	}

	printf("PUT: %s success!\n", fileName);
	free(part1);
	free(part2);
	free(part3);
	free(part4);
}

void LIST(struct configuration myCredentials){
	int dfs1, dfs2, dfs3, dfs4;
	char *aBUF = calloc(BUFFERSIZE, 1);
	char *host = "localhost";
	char *request = calloc(BUFFERSIZE, 1);
	char filesRecieved[20][100];
	char *fileRecieved = calloc(BUFFERSIZE, 1);
	char *size = calloc(BUFFERSIZE, 1);
	int sizeOfFile;
	strcat(request, "LIST:");
	strcat(request, ":");
	strcat(request, "\0");

	char *ack = calloc(BUFFERSIZE, 1); 
	int index = 0;
	// Server 1
	dfs1 = connect_socket(host, myCredentials.DFS1);
	if(dfs1 != -1){
		send(dfs1, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);	
		read(dfs1, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			send(dfs1, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
			memset(aBUF, 0, BUFFERSIZE);
			read(dfs1, aBUF, BUFFERSIZE);
			
			if(!strstr(aBUF, "Invalid"))
			{
				send(dfs1, request, strlen(request), 0);
				read(dfs1, ack, BUFFERSIZE);
				if(strstr(ack, "ACK")){
					sizeOfFile = BUFFERSIZE;
					while(!strstr(fileRecieved, "DONE")){
						read(dfs1, size, sizeOfFile);
						if(strstr(size, "DONE")){
							break;
						}
						sizeOfFile = atoi(size);
						send(dfs1, "ACK SIZE", 8, 0);
						read(dfs1, fileRecieved, sizeOfFile);
						send(dfs1, "ACK FILE", 8, 0);
						while(strlen(fileRecieved) != sizeOfFile){
							fileRecieved[strlen(fileRecieved)-1] = 0;
						}
						strcpy(filesRecieved[index], fileRecieved);
						index++;
					}
					send(dfs1, "ACK DONE", 8, 0);
				}
			}
		}
	}
	close(dfs1);

	// Server 2
	dfs2 = connect_socket(host, myCredentials.DFS2);
	if(dfs2 != -1){		
		send(dfs2, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);	
		read(dfs2, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			send(dfs2, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
			memset(aBUF, 0, BUFFERSIZE);
			read(dfs2, aBUF, BUFFERSIZE);
			
			if(!strstr(aBUF, "Invalid"))
			{
				send(dfs2, request, strlen(request), 0);
				read(dfs2, ack, BUFFERSIZE);
				if(strstr(ack, "ACK")){
					sizeOfFile = BUFFERSIZE;
					while(!strstr(fileRecieved, "DONE")){
						read(dfs2, size, sizeOfFile);
						if(strstr(size, "DONE")){
							break;
						}
						sizeOfFile = atoi(size);
						send(dfs2, "ACK SIZE", 8, 0);
						read(dfs2, fileRecieved, sizeOfFile);
						send(dfs2, "ACK FILE", 8, 0);
						while(strlen(fileRecieved) != sizeOfFile){
							fileRecieved[strlen(fileRecieved)-1] = 0;
						}
						strcpy(filesRecieved[index], fileRecieved);
						index++;
					}
					send(dfs1, "ACK DONE", 8, 0);
				}
			}
		}
	}
	close(dfs2);

	// Server 3
	dfs3 = connect_socket(host, myCredentials.DFS3);
	if(dfs3 != -1){
		send(dfs3, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);	
		read(dfs3, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			send(dfs3, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
			memset(aBUF, 0, BUFFERSIZE);
			read(dfs3, aBUF, BUFFERSIZE);
			
			if(!strstr(aBUF, "Invalid"))
			{
				send(dfs3, request, strlen(request), 0);
				read(dfs3, ack, BUFFERSIZE);
				if(strstr(ack, "ACK")){
					sizeOfFile = BUFFERSIZE;
					while(!strstr(fileRecieved, "DONE")){
						read(dfs3, size, sizeOfFile);
						if(strstr(size, "DONE")){
							break;
						}
						sizeOfFile = atoi(size);
						send(dfs3, "ACK SIZE", 8, 0);
						read(dfs3, fileRecieved, sizeOfFile);
						send(dfs3, "ACK FILE", 8, 0);
						while(strlen(fileRecieved) != sizeOfFile){
							fileRecieved[strlen(fileRecieved)-1] = 0;
						}
						strcpy(filesRecieved[index], fileRecieved);
						index++;
					}
					send(dfs3, "ACK DONE", 8, 0);
				}
			}
		}
	}
	close(dfs3);

	// Server 4
	dfs4 = connect_socket(host, myCredentials.DFS4);
	if(dfs4 != -1){
		send(dfs4, myCredentials.myUSERNAME, strlen(myCredentials.myUSERNAME), 0);	
		read(dfs4, aBUF, BUFFERSIZE);

		if(!strstr(aBUF, "Invalid"))
		{
			send(dfs4, myCredentials.myPASSWORD, strlen(myCredentials.myPASSWORD), 0);
			memset(aBUF, 0, BUFFERSIZE);
			read(dfs4, aBUF, BUFFERSIZE);
			
			if(!strstr(aBUF, "Invalid"))
			{
				send(dfs4, request, strlen(request), 0);
				read(dfs4, ack, BUFFERSIZE);
				if(strstr(ack, "ACK")){
					sizeOfFile = BUFFERSIZE;
					while(!strstr(fileRecieved, "DONE")){
						read(dfs4, size, sizeOfFile);
						if(strstr(size, "DONE")){
							break;
						}
						sizeOfFile = atoi(size);
						send(dfs4, "ACK SIZE", 8, 0);
						read(dfs4, fileRecieved, sizeOfFile);
						send(dfs4, "ACK FILE", 8, 0);
						while(strlen(fileRecieved) != sizeOfFile){
							fileRecieved[strlen(fileRecieved)-1] = 0;
						}
						strcpy(filesRecieved[index], fileRecieved);
						index++;
					}
					send(dfs4, "ACK DONE", 8, 0);
				}
			}
		}
	}
	close(dfs4);

	int i, j, k;
	char filesFound[20][100];
	char *currentFile = calloc(BUFFERSIZE, 1);
	char *currentFileNum = calloc(BUFFERSIZE, 1);
	char *currentFileRoot = calloc(BUFFERSIZE, 1);
	char *file1 = calloc(BUFFERSIZE, 1);
	char *file2 = calloc(BUFFERSIZE, 1);
	char *file3 = calloc(BUFFERSIZE, 1);
	int fileSeenFlag = 0;
	int numFound = 0;
	int file1Flag = 0;
	int file2Flag = 0;
	int file3Flag = 0;
	for(i = 0; i<index; i++){
		fileSeenFlag = 0;
		strcpy(currentFile, filesRecieved[i]);
		strcpy(currentFileRoot, currentFile);
		currentFileRoot[strlen(currentFileRoot)-1] = 0;
		strcpy(file1, currentFileRoot);
		strcpy(file2, currentFileRoot);
		strcpy(file3, currentFileRoot);
		strcpy(currentFileNum, (char*)(currentFile+strlen(currentFile)-1));

		for(k=0;k<numFound; k++){
			if(strstr(filesFound[k], currentFileRoot)){
				fileSeenFlag = 1;
			}
		}
		if(fileSeenFlag != 1){
			numFound++;
			if(strstr(currentFileNum, "1")){
				strcat(file1, "2");
				strcat(file2, "3");
				strcat(file3, "4");
			}
			else if(strstr(currentFileNum, "2")){
				strcat(file1, "1");
				strcat(file2, "3");
				strcat(file3, "4");			
			}
			else if(strstr(currentFileNum, "3")){
				strcat(file1, "1");
				strcat(file2, "2");
				strcat(file3, "4");			
			}
			else if(strstr(currentFileNum, "4")){
				strcat(file1, "1");
				strcat(file2, "2");
				strcat(file3, "3");			
			}
			//FILE 1
			for(j=i; j<index && file1Flag == 0;j++){
				if(strstr(file1, filesRecieved[j])){
					file1Flag = 1;
				}
			}
			//FILE 2
			for(j=i; j<index && file2Flag == 0;j++){
				if(strstr(file2, filesRecieved[j])){
					file2Flag = 1;
				}
			}
			//FILE 3
			for(j=i; j<index && file3Flag == 0;j++){
				if(strstr(file3, filesRecieved[j])){
					file3Flag = 1;
				}
			}
			strcpy(filesFound[i], currentFileRoot);
			currentFileRoot[strlen(currentFileRoot)-1] = 0;
			if(file1Flag == 1 && file2Flag == 1 && file3Flag == 1){
				printf("%s [COMPLETE]\n\n", currentFileRoot);
			}
			else{
				printf("%s [INCOMPLETE]\n\n", currentFileRoot);
			}
		}
	}
}

int main(int argc, char* argv[]){
	int count = 0;
	struct configuration myCredentials;
	char *buffer = calloc(BUFFERSIZE, 1);
	char *buf, *line, *currentLine, *request;
	char *fileName = "\0";

	myCredentials = parse_config_file(argv[1]);
	// START READING INPUT
	while (fgets(buffer, BUFFERSIZE, stdin)) {
		buffer[strlen(buffer)-1] = '\0';
		buf = buffer;
		// PARSE INPUT
		while ((line = strsep(&buf, "\0")) != NULL){
			while((currentLine = strsep(&line, " ")) != NULL){
				if (count == 0)
					request = currentLine;
				else
					fileName = currentLine;
				count++;
			}
			count = 0;
		}
		if (strcmp(request, "GET") == 0){
			if(strcmp(fileName, "\0") != 0){
				GET(myCredentials, fileName);
			}
			else
				printf("Error: Need to provide file name\n");	
		}
		else if (strcmp(request, "PUT") == 0){	
			if(strcmp(fileName, "\0") != 0)
				PUT(myCredentials, fileName);
			else
				printf("Error: Need to provide file name\n");
		}
		else if (strcmp(request, "LIST") == 0)
			LIST(myCredentials);
		else
			printf("Error: Invalid request\n");
		fileName = "\0";
	}
	return 0;
}