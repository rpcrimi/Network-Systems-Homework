#ifndef HTTPREQUEST_H_   /* Include guard */
#define HTTPREQUEST_H_
struct Http_Request {
	char 	method[256];
	char 	url[256];
	char 	http_version[256];
	char 	host[128];
	int 	keep_alive;
} http_request;
#endif