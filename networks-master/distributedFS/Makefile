CC = gcc
CFLAGS = -Wall -g -lpthread 
client_dir = client/dfc.c
server_dir = server/dfs.c
server_commands = server/commands.c

all: dfc dfs 

dfc:
	$(CC) -o dfc $(client_dir) $(CFLAGS) 

dfs: 
	$(CC) -o dfs $(server_dir) $(CFLAGS) 

clean:
	rm -f dfc dfs *.o *~ core