#Simple web server
Code inspired from: tinyhttpd-0.1.0 and rosetta code.

Compiled on a macintosh with gcc. 

###To run: 
	make clean
	make
	./webserver

###ws.conf
* Document root is not the full path. Instead it is just the folder where html resides. So in this case: www
* All other parsing works as normal.

###Web server currently handles: 
* png, text, jpg, and html files
* 500, 501, 404, and 400 errors


To test pipeline support, set the port to any number higher than 1024 and then run the
following command replacing <your port number> with the one you chose:

```bash
(echo -en "GET /index.html HTTP/1.1\n Host: localhost \nConnection: keep-alive\n\nGET
/index.html HTTP/1.1\nHost: localhost\n\n"; sleep 10) | telnet localhost <your port number>
```
