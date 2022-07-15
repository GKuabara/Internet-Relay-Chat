#include "socket.h"

int check(int exp, const char *msg) {
    if (exp == SOCKETERROR) {
        perror(msg);
        exit(1);
    }
    return exp;
}

// Function to send multiple messages
void* send_thread(void *args) {
    int* client_sock = (int*) args;
    char msg[MSG_LEN];
    memset(msg, 0, MSG_LEN);

    while (1) {
        fgets(msg, MSG_LEN, stdin);
        if (msg == NULL) break;
        if (msg[0] == '\n') continue;
        send(*client_sock, msg, strlen(msg), 0);
        printf("msg len: %ld\n", strlen(msg));
        memset(msg, 0, MSG_LEN);
    }
}

// Function to receive multiple messages
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
    return NULL;
}