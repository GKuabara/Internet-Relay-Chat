/*
Alunos:
- Gabriel Alves Kuabara - nUSP 11275043
- Guilherme Lourenço de Toledo - nUSP 11795811
- Victor Henrique de Sa Silva - nUSP 11795759
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MSG_LEN 4096
#define PORT 1337

int send_all(int *soc, char *buf, int *len) {
    int total = 0; // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    while(total < *len) {
        n = send(*soc, buf + total, bytesleft, 0);
        if (n == -1) break;
        total += n;
        bytesleft -= n;
    }
    *len = total; // return number actually sent here
    return n==-1?-1:0; // return -1 on failure, 0 on success
}

void* send_thread(void *args) {
    int* client_sock = (int*) args;
    char msg[MSG_LEN];
    memset(msg, 0, MSG_LEN);

    while (1) {
        fgets(msg, MSG_LEN, stdin);
        if (msg == NULL) break;
        if (msg[0] == '\n') continue;
        int slen = strlen(msg);
        //if (slen < MSG_LEN) msg[slen] = '\0';
        int n_msgs = (slen / MSG_LEN) + 1;
        //int bytes_read = send(*client_sock, msg, strlen(msg), 0);
        if (send_all(client_sock, msg, &slen) == -1) break;
        printf("msg len: %d,\tnumber of msg: %d\n", slen, n_msgs);
        memset(msg, 0, MSG_LEN);
    }
}

void* receive_thread(void *args) {
    int* client_sock = (int*) args;
    char client_message[MSG_LEN];
    ssize_t bytes_received;

    while (1) {
        bytes_received = recv(*client_sock, client_message, MSG_LEN, 0);
        if (bytes_received == 0) break;
        if (bytes_received <= -1) {
            perror("Error on connection");
            break;
        }
        if (strcmp(client_message, "exit\n") == 0) {
            send(*client_sock, client_message, strlen(client_message), 0); // sending message saying that the connection is being closed
            printf("Closing connection\n");
            break;
        }
        printf("bytes received:%ld\n", bytes_received);
        printf("%s", client_message);
        memset(client_message, 0, MSG_LEN);
    }
}

int main(void) {
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0) {
        printf("Unable to create socket\n");
        return -1;
    }
    if  (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    printf("Socket created successfully\n");
    
    // Set port and IP the same as server-side:
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Send connection request to server:
    if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Unable to connect\n");
        return -1;
    }
    printf("Connected with server successfully\n");
    
    // creating thread to send messages
    pthread_t thread_send;
    pthread_create(&thread_send, NULL, &send_thread, &socket_desc);
    receive_thread(&socket_desc);

    pthread_cancel(thread_send);

    // Close the socket:
    close(socket_desc);
    return 0;
}