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
    SA_IN address;
    int sockfd;
    int uid;
    char name[NAME_LEN];
    int is_admin;
    int is_muted;
} client_t;

typedef struct {
    char key[CNL_LEN];
    client_t* connected_cli[MAX_CLIENTS];
    int n_cli;
    int admin_id;
} channel_t;

client_t* clients[MAX_CLI_PER_CHANNEL];
channel_t* channels[MAX_CHANNELS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t channels_mutex = PTHREAD_MUTEX_INITIALIZER;

int find_client (client_t* cli) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i] == cli) {
            return i;
        }
    }
    return -1;
}

int find_channel_of_client(client_t* cli) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i]) {

            for (int j = 0; j < MAX_CLI_PER_CHANNEL; j++) {
                if (channels[i]->connected_cli[j] && channels[i]->connected_cli[j] == cli) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int find_channel_and_client (char* str, int* idx) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i]) {

            for (int j = 0; j < MAX_CLI_PER_CHANNEL; j++) {
                if (channels[i]->connected_cli[j] && strcmp(channels[i]->connected_cli[j]->name, str) == 0) {
                    *idx = j;
                    return i;
                }
            }
        }
    }
    return -1;
}

int find_channel(char* str) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i] && strcmp(channels[i]->key, str) == 0)
            return i;
    }
    return -1;
}

/* Add clients to channel */
int add_client_to_channel(char* str, client_t* cli) {
	pthread_mutex_lock(&channels_mutex);

    if (str[0] != '#' && str[0] != '&') {
	    pthread_mutex_unlock(&channels_mutex);
        return 0;
    }

    int idx;
    if ((idx = find_channel(str)) > -1) {
        for (int i = 0; i < MAX_CLI_PER_CHANNEL; ++i){
            if(!channels[idx]->connected_cli[i]) {
                channels[idx]->connected_cli[i] = cli;
                channels[idx]->n_cli++;
                pthread_mutex_unlock(&channels_mutex);
                return 1;
            }                          
        }
	}
    else {
        channel_t* new_cnl = malloc(sizeof(*new_cnl));
        strcpy(new_cnl->key, str);
        new_cnl->admin_id = cli->uid;
        new_cnl->connected_cli[0] = cli;
        new_cnl->n_cli = 1;
        for (int i = 0; i < MAX_CHANNELS; i++) {
            if(!channels[i]) {
                channels[i] = new_cnl;
                pthread_mutex_unlock(&channels_mutex);
                return 1;
            }
        }
    }
	pthread_mutex_unlock(&channels_mutex);
    return 0;
}

/* Remove client from channel */
void remove_client_from_channel(int idx, client_t* cli) {
	pthread_mutex_lock(&channels_mutex);
    
    for (int i = 0; i < MAX_CLI_PER_CHANNEL; i++) {
        if (channels[idx]->connected_cli[i] && channels[idx]->connected_cli[i] == cli) {
            channels[idx]->connected_cli[i] = NULL;
            channels[idx]->n_cli--;
            break;
        }
    }
    if (channels[idx]->n_cli == 0) {
        free(channels[idx]);
    }

	pthread_mutex_unlock(&channels_mutex);
}

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
    if (cli->is_muted) return;
	pthread_mutex_lock(&clients_mutex);

    int idx;
    if ((idx = find_channel_of_client(cli)) < 0) {
        pthread_mutex_unlock(&clients_mutex);
        return;
    }

	for(int i = 0; i < MAX_CLI_PER_CHANNEL; ++i){
		if(channels[idx]->connected_cli[i] && channels[idx]->connected_cli[i] != cli){
            
            int bytes_sent = check(write(channels[idx]->connected_cli[i]->sockfd, s, strlen(s)),"ERROR: write to descriptor failed");
                
            if (bytes_sent == 0) {
                close(channels[idx]->connected_cli[i]->sockfd);
                queue_remove(channels[idx]->connected_cli[i]->uid);
                remove_client_from_channel(idx, cli);
            }
        }
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
        if (flag_leave || !cli) break;
        str_overwrite_stdout();
        
        int command = 0;
        int receive = recv(cli->sockfd, msg, MSG_LEN, 0);
        if (receive > 0) {
            str_trim_lf(msg);

            int n_tokens = 0;
            char** tokens = str_get_tokens_(msg, ' ');
            for (int i = 0; tokens[i]; i++) {
                //printf("%d: \'%s\'\n", i, tokens[i]);
                n_tokens++;
            }

            if (strcmp(msg, "/ping") == 0) {
                printf("command: ping\n");
                check(write(cli->sockfd, "server: pong\n", strlen("server: pong\n")),"ERROR: write to descriptor failed");
                sprintf(buffer_out, "%s: %s\n", name, msg);
                //str_trim_lf(buffer_out);
                printf("%s", buffer_out);
            }
            if (n_tokens == 2) {
                if (strcmp(tokens[0], "/join") == 0) {
                    printf("command: join\n");
                    if (add_client_to_channel(tokens[1], cli)) {

                        sprintf(buffer_out, "%s joined channel %s\n", name, tokens[1]);
                        check(write(cli->sockfd, buffer_out, BUFF_LEN), "ERROR: write to descriptor failed");
                        //str_trim_lf(buffer_out);
                        printf("%s", buffer_out);
                    }
                    else {
                        sprintf(buffer_out, "channel %s is invalid or full\n", tokens[1]);
                        check(write(cli->sockfd, buffer_out, BUFF_LEN), "ERROR: write to descriptor failed");
                        //str_trim_lf(buffer_out);
                        printf("%s", buffer_out);
                    }
                    command = 1;
                }
                else if (strcmp(tokens[0], "/nickname") == 0) {
                    printf("command: nickname\n");
                    printf("%s nickname updated to ", cli->name);
                    strcpy(cli->name, tokens[1]);
                    printf("%s\n", cli->name);
                    command = 1;
                }
                else if (cli->is_admin) {
                    printf("command: admin\n");
                    int adm_idx;
                    int adm_cnl = find_channel_and_client(cli->name, &adm_idx);

                    int cli_idx;
                    int cnl_idx = find_channel_and_client(tokens[1], &cli_idx);

                    if (cnl_idx != adm_cnl || cnl_idx < 0) continue; 

                    if (strcmp(tokens[0], "/kick") == 0) {
                        printf("command: kick\n");
                        int i = find_client(channels[cnl_idx]->connected_cli[cli_idx]);

                        close(clients[i]->sockfd);
                        queue_remove(clients[i]->uid);
                        remove_client_from_channel(cnl_idx, clients[i]);
                        free(clients[i]);
                        command = 1;
                        break;
                    }
                    else if (strcmp(tokens[0], "/mute") == 0) {
                        printf("command: mute\n");
                        channels[cnl_idx]->connected_cli[cli_idx]->is_muted = 1;
                        command = 1;
                    }
                    else if (strcmp(tokens[0], "/unmute") == 0) {
                        printf("command: unmute\n");
                        channels[cnl_idx]->connected_cli[cli_idx]->is_muted = 0;
                        command = 1;
                    }
                    else if (strcmp(tokens[0], "/whois") == 0) {
                        printf("command: whois\n");
                        sprintf(buffer_out, "%s is %s\n", tokens[1], inet_ntoa(channels[cnl_idx]->connected_cli[cli_idx]->address.sin_addr));
                        check(write(channels[cnl_idx]->connected_cli[cli_idx]->sockfd, buffer_out, BUFF_LEN),"ERROR: write to descriptor failed");
                        command = 1;
                    }
                }
            }
            if (!command) {
                sprintf(buffer_out, "%s: %s\n", name, msg);
                send_message(buffer_out, cli);
                //str_trim_lf(buffer_out);
                printf("%s",buffer_out);
            }
            //str_overwrite_stdout();
            str_free_tokens(tokens);
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

    if (cli) {
        int idx = find_channel_of_client(cli);
        close(cli->sockfd);
        queue_remove(cli->uid);
        if (idx != -1) remove_client_from_channel(idx, cli);
        free(cli);
        pthread_detach(pthread_self());
    }
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
        cli->is_admin = 0;
        cli->is_muted = 0;
        
        queue_add(cli);
        // for (int i = 0; i < MAX_CLIENTS; i++) {
        //     if (clients[i]) {
        //         printf("sockfd: %d, uid: %d, name: %s\n", clients[i]->sockfd, clients[i]->uid, clients[i]->name);
        //     }
        // }
        pthread_create(&thread, NULL, &handle_client,(void*)cli);

        sleep(1);
    }

    // Closing the socket:
    close(server_socket);
    return 0;
}