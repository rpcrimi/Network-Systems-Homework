#ifndef CONFIG_H_   /* Include guard */
#define CONFIG_H_
typedef struct server {
    char host[20];
    int port;
    int fd;
} server;
#endif