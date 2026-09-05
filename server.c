#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 4096
#define USERNAME_SIZE 20
#define PASSWORD_SIZE 6
#define MAX_CLIENTS 10

typedef struct{
    int socket;
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
}client_data;

int count = 0;
int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void* client_thread(void* args) {
    client_data *cd = (client_data*)args;

    fprintf(stdout, "A client has logged in\n");
    memset(cd->username, 0, USERNAME_SIZE * sizeof(char));;
    if(recv(cd->socket, cd->username, USERNAME_SIZE * sizeof(char), 0) == -1) {
        perror("Failed to receive username!");
        close(cd->socket);
        exit(-1);
    }

    memset(cd->password, 0, PASSWORD_SIZE * sizeof(char));
    if(recv(cd->socket, cd->password, PASSWORD_SIZE * sizeof(char), 0) == -1) {
        perror("Failed to receive password!");
        close(cd->socket);
        exit(-1);
    }

    char *buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    while(1) {
        memset(buffer, 0, BUFFER_SIZE * sizeof(char));

        ssize_t received_bytes = recv(cd->socket, buffer, BUFFER_SIZE * sizeof(char), 0);
        if(received_bytes == -1) {
            perror("Failed to receive message!");
            close(cd->socket);
            exit(-1);
        }
        else if(received_bytes == 0) {
            perror("0 bytes were sent, closing ...");
            close(cd->socket);
            exit(-1);
        }
        buffer[strlen(buffer) - 1] = '\0';
        fprintf(stdout, "\r\033[2K[%s] -> %s (bytes %zd)\n", cd->username, buffer, received_bytes);

        pthread_mutex_lock(&clients_mutex);
        for(int i = 0; i < count; i++) {
            if(client_sockets[i] != cd->socket) {
                if(send(client_sockets[i], buffer, strlen(buffer), 0) == -1) {
                    fprintf(stdout, "%d customer didnt receive message", i);
                }

                usleep(100000);

                if(send(client_sockets[i], cd->username, strlen(cd->username), 0) == -1) {
                    fprintf(stdout, "%d customer didnt receive message", i);
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        fprintf(stdout, "usage -> <./executable> <PORT number>\n");
        exit(-1);
    }

    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverfd == -1) {
        perror("Failed to create socket!");
        exit(-1);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));

    int addrsize = sizeof(address);
    if(bind(serverfd, (const struct sockaddr*)&address, addrsize) == -1) {
        perror("Failed to bind server!");
        close(serverfd);
        exit(-1);
    }

    if(listen(serverfd, MAX_CLIENTS) == -1) {
        perror("Server failed to listen!");
        close(serverfd);
        exit(-1);
    }

    client_data *cd;
    int clientfd;
    while(1) {
        clientfd = accept(serverfd, (struct sockaddr*)&address, (socklen_t*)&addrsize);
        if(clientfd == -1) {
            perror("Failed to accpet the client!");
            close(serverfd);
            close(clientfd);
            exit(-1);
        }
        pthread_mutex_lock(&clients_mutex);
        if(count <= MAX_CLIENTS) {
            pthread_t client_handler;
            cd = (client_data*)malloc(sizeof(client_data));
            cd->socket = clientfd;

            client_sockets[count++] = clientfd;

            pthread_create(&client_handler, NULL, client_thread, cd);

            pthread_detach(client_handler);
        }
        else {
            fprintf(stdout, "Server is full, try again later...\n");
            close(clientfd);
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    free(cd);
    close(serverfd);
    close(clientfd);
    return 0;
}