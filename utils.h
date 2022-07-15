#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h> 
#include <signal.h>

int clear_icanon();
int send_all(int *soc, char *buf, int *len);
void str_overwrite_stdout();
void str_trim_lf (char* arr);

#endif