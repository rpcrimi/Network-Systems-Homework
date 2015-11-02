# README

Robert Crimi
Network Systems PA2
Distributed File Server

To compile:
make clean
make

To run:
./dfc dfc.conf
./dfs DFS1 10001 &
./dfs DFS2 10002 &
./dfs DFS3 10003 &
./dfs DFS4 10004 &

You can now begin to type commands

Commands should be formatted as such:
PUT "filename"
GET "filename"
LIST

For example,
PUT test.txt
GET test.txt
LIST


Things that work:
All three commands are fully working, however I have not implemented subfolders.
Encryption is working by using XOR encryption with the password in dfc.conf. Files stay encrypted on the server side and are combined and decrypted once sent back to the client.