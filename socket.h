#ifndef SOCKET_H
#define SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h> 
#include <limits.h>

#define MSG_LEN 4096
#define NAME_LEN 50
#define BUFF_LEN (MSG_LEN + NAME_LEN + 4)
#define PORT 1337
#define SERVERPORT 1337
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 100
#define MAX_CLIENTS 20

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

int check(int exp, const char *msg);
void* send_thread(void *args);
void* receive_thread(void *args);

#endif