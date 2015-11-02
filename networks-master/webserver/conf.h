#ifndef CONF_H_   /* Include guard */
#define CONF_H_
struct Config {
    int    port;
    char    document_root[256];
    char    directory_index[256];
    char    contentType[100];
} conf;
#endif