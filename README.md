# Internet-Relay-Chat

# Students

- Gabriel Alves Kuabara - nUSP 11275043
- Guilherme Lourenço de Toledo - nUSP 11795811
- Victor Henrique de Sa Silva - nUSP 11795759

# Project Description

Project built to "SSC0142 - Computer Networks" course during 1st semester of 2022 at ICMC - Univeristy of São Paulo.

This project have branches with diferents deliveries.

Throughout this project we implemented the various parts that make up an IRC, or Internet Relay Chat, client and server, widely used in the 90's and still used today by some computer groups. The implementation to be done is an adaptation of the specifications given by RFC 1459, which defines IRC. The IRC protocol has been developed in systems using the TCP/IP protocol and is a system that must support multiple clients connected to a single server, performing multiplexing of the data received by them.

[Explicative Video]([https://www.google.com](https://youtu.be/fEpNcORgKRM))

The code was enterely built in programing lanaguage C and in a Ubuntu 22.04 Operational System. Also, the implementation uses gcc (version 11.2.0) and make (version 4.3) packages.


The code can be compiled using any starndad Linux with gcc and make packages through the following command:

```
make all
```

To run the code, first run the server:

```
./server
```

To start a client:

```
./client
```
