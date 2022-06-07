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

void* send_thread(void *args) {
    int* client_sock = (int*) args;
    char msg[MSG_LEN];
    memset(msg, 0, MSG_LEN);

    while (1) {
        fgets(msg, MSG_LEN, stdin);
        if (msg == NULL) break;
        if (msg[0] == '\n') continue;
        int bytes_read = send(*client_sock, msg, strlen(msg), 0);
        printf("msg len: %ld\n", strlen(msg));
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
            // sending message saying that the connection is being closed
            send(*client_sock, client_message, strlen(client_message), 0);
            printf("Closing connection\n");
            break;
        }
        printf("bytes received:%ld\n", bytes_received);
        printf("%s", client_message);
        memset(client_message, 0, MSG_LEN);
    }
}

int main() {
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    if  (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    printf("Socket created successfully\n");
    
    // Set port and IP:
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");
    
    // Listen for clients:
    if(listen(socket_desc, 1) < 0) {
        printf("Error while listening\n");
        return -1;
    }
    printf("\nListening for incoming connections.....\n");
    
    // Accept an incoming connection:
    struct sockaddr_in client_addr;
    int client_size = sizeof(client_addr);
    int client_sock = accept(socket_desc, (struct sockaddr*)&client_addr, &client_size);
    
    if (client_sock < 0){
        printf("Can't accept\n");
        return -1;
    }
    if  (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // creating thread to send messages
    pthread_t thread_send;
    pthread_create(&thread_send, NULL, &send_thread, &client_sock);
    receive_thread(&client_sock);

    pthread_cancel(thread_send);

    // Closing the socket:
    close(client_sock);
    close(socket_desc);
    return 0;
}