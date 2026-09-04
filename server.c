#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 4096
#define MAX_CLIENTS 10

int count = 0;
int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void* client_thread(void* args) {
    fprintf(stdout, "A client has logged in\n");
    char *buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);

    int clientfd = *(int*)args;

    while(1) {
        memset(buffer, 0, BUFFER_SIZE * sizeof(char));

        ssize_t received_bytes = recv(clientfd, buffer, BUFFER_SIZE * sizeof(char), 0);
        if(received_bytes == -1) {
            perror("Failed to receive message!");
            close(clientfd);
            exit(-1);
        }
        else if(received_bytes == 0) {
            perror("0 bytes were sent, closing ...");
            close(clientfd);
            exit(-1);
        }
        buffer[strlen(buffer) - 1] = '\0';
        fprintf(stdout, "\r\033[2K[Client num] -> %s (bytes %zd)\n", buffer, received_bytes);

        for(int i = 0; i < count; i++) {
            if(send(client_sockets[i], buffer, strlen(buffer), 0) == -1) {
                fprintf(stdout, "%d customer didnt receive message", i);
            }
        }
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

    int clientfd;
    while(1) {
        if(count <= MAX_CLIENTS) {
            clientfd = accept(serverfd, (struct sockaddr*)&address, (socklen_t*)&addrsize);
            if(clientfd == -1) {
                perror("Failed to accpet the client!");
                close(serverfd);
                close(clientfd);
                exit(-1);
            }
            pthread_t client_handler;

            client_sockets[count] = clientfd;

            pthread_create(&client_handler, NULL, client_thread, &client_sockets[count++]);

            pthread_detach(client_handler);
        }
    }
    close(serverfd);
    close(clientfd);
    return 0;
}