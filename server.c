/*
Alunos:
- Gabriel Alves Kuabara - nUSP 11275043
- Guilherme Lourenço de Toledo - nUSP 11795811
- Victor Henrique de Sa Silva - nUSP 11795759
*/

#include "utils.h"
#include "socket.h"

static _Atomic unsigned int n_clients = 0;
static int uid = 10;

typedef struct {
    char key[CNL_LEN];
    
} channel_t;

typedef struct {
    SA_IN address;
    int sockfd;
    int uid;
    char name[NAME_LEN];
} client_t;

client_t* clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Add clients to queue */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);
	for(int i = 0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);
	for(int i = 0; i < MAX_CLIENTS; ++i){
		if(clients[i] && clients[i]->uid == uid){
            clients[i] = NULL;
            break;
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, client_t* cli){
	pthread_mutex_lock(&clients_mutex);
    char msg[9];

	for(int i = 0; i < MAX_CLIENTS; ++i){
		if(clients[i] && clients[i]->uid != cli->uid){
            // try to send message 5 times, if can't close connection
            // int c = 5;
            // while(c--) {
                check(write(clients[i]->sockfd, s, strlen(s)),"ERROR: write to descriptor failed");
                
            //     int bytes_recv = recv(clients[i]->sockfd, msg, sizeof(msg), 0);
            //     str_trim_lf(msg);
            //     if (bytes_recv > 0 && strcmp(msg, "received") == 0)
            //         break;
            // }
            // if (c == 0) {
            //     close(clients[i]->sockfd);
            //     queue_remove(clients[i]->uid);
            // }
        }
        bzero(msg, sizeof(msg));
	}
	pthread_mutex_unlock(&clients_mutex);
}

/* Build server socket */
int setup_server(int port, int backlog, SA_IN* server_addr) {
    int server_socket;

    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create socket!");

    // initialize the address struct
    (*server_addr).sin_family = AF_INET;
    (*server_addr).sin_port = htons(SERVERPORT);
    (*server_addr).sin_addr.s_addr = inet_addr("127.0.0.1");

    signal(SIGPIPE, SIG_IGN);
    check(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)), "setsockopt(SO_REUSEADDR) failed");

    check(bind(server_socket, (SA*)server_addr, sizeof(*server_addr)), "Bind Failed");
    check(listen(server_socket, backlog), "Listen Failed");

    printf("Listening on: %s:%d\n", inet_ntoa((*server_addr).sin_addr), ntohs((*server_addr).sin_port));

    return server_socket;
}

/* Accept connection */
int accept_new_connection(int server_socket, SA_IN* client_addr) {
    socklen_t addr_size = sizeof(*client_addr);
    int client_socket;
    
    check(client_socket = accept(server_socket, (SA*)client_addr, &addr_size), "Accept failed");

    check(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)), "setsockopt(SO_REUSEADDR) failed");

    // Check limit number of clients
    if((n_clients + 1) == MAX_CLIENTS) {
        printf("Max clients reached, bye bye: ");
        printf("%s:%d\n", inet_ntoa((*client_addr).sin_addr), ntohs((*client_addr).sin_port));
        close(client_socket);
        return -1;
    }

    printf("Client connected at IP: %s and port: %d\n", inet_ntoa((*client_addr).sin_addr), ntohs((*client_addr).sin_port));
    
    n_clients++;
    return client_socket;
}

/* Thread function to handle each client */
void* handle_client(void *arg) {
    char msg[MSG_LEN];
    char buffer_out[BUFF_LEN];
    char name[NAME_LEN];
    int flag_leave = 0;
    client_t *cli = (client_t *) arg;

    // get client name
    if (recv(cli->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LEN - 1) {
        printf("Failed to get name or invalid name\n");
        flag_leave = 1;
    } else {
        strcpy(cli->name, name);
        sprintf(buffer_out, "%s has joined chat\n", cli->name);
        printf("%s", buffer_out);
        send_message(buffer_out, cli);
    }

    bzero(buffer_out, BUFF_LEN);

    while (1) {
        if (flag_leave) break;

        int receive = recv(cli->sockfd, msg, BUFF_LEN, 0);
        if (receive > 0) {
            str_trim_lf(msg);

            if (strcmp(msg, "/ping") == 0) {
                check(write(cli->sockfd, "server: pong", strlen("server: pong")),"ERROR: write to descriptor failed");
                sprintf(buffer_out, "%s: %s\n", name, msg);
                str_trim_lf(buffer_out);
                printf("> %s\n", buffer_out);
            }
            else {
                sprintf(buffer_out, "%s: %s\n", name, msg);
                send_message(buffer_out, cli);
                str_trim_lf(buffer_out);
                printf("> %s\n",buffer_out);
            }
        }
        else if(receive == 0 || strcmp(msg, "/quit") == 0) {
            sprintf(buffer_out, "%s has left\n", cli->name);
            printf("%s", buffer_out);
            send_message(buffer_out, cli);
            flag_leave = 1;
        }
        else {
            printf("ERROR: -1\n");
            flag_leave = 1;
        }
        bzero(msg, MSG_LEN);
        bzero(buffer_out, BUFF_LEN);
    }

    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    pthread_detach(pthread_self());
    return NULL;
}

int main() {
    clear_icanon();
    SA_IN* client_addr = (SA_IN*)malloc(sizeof(SA_IN));
    SA_IN* server_addr = (SA_IN*)malloc(sizeof(SA_IN));
    pthread_t thread;

    int server_socket = setup_server(SERVERPORT, SERVER_BACKLOG, server_addr);

    printf("=== WELCOME TO ZAP ZAP ===\n");

    while (1) {
        int client_socket;
        if((client_socket = accept_new_connection(server_socket, client_addr)) < 0) continue;

        // creating thread to send messages
        client_t* cli = (client_t *) malloc(sizeof(client_t));
        cli->address = *client_addr;
        cli->sockfd = client_socket;
        cli->uid = uid++;
        
        queue_add(cli);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i]) {
                printf("sockfd: %d, uid: %d, name: %s\n", clients[i]->sockfd, clients[i]->uid, clients[i]->name);
            }
        }
        pthread_create(&thread, NULL, &handle_client,(void*)cli);

        sleep(1);
    }

    // Closing the socket:
    close(server_socket);
    return 0;
}