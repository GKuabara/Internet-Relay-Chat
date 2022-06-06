# First Part

# Project Description

This part of the project consists in building a communication between a server and an unique client through sockets. Both host and server must be able to send and receive multiple messages in any sequence.

The code was enterely built in programing lanaguage C and in a Ubuntu 22.04 Operational System. Also, the implementation uses gcc (version 11.2.0) and make (version 4.3) packages.

The code can be compiled using any starndad Linux with gcc package through the following commands:

```c
// compile the server and the client
gcc server.c -o server
gcc client.c -o client
```

To run the code, first run the server, then the client:
```c
./server
./client
```