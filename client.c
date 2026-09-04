#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 4096

typedef struct {
    char *buffer;
    int socket;
}thread_data;

void* receive_thread(void* args) {
    thread_data* d = (thread_data*)args;
    while(1) {
        memset(d->buffer, 0, BUFFER_SIZE * sizeof(char));

        ssize_t received_bytes = recv(d->socket, d->buffer, BUFFER_SIZE * sizeof(char), 0);
        if(received_bytes == -1) {
            perror("Failed to receive message!");
            close(d->socket);
            exit(-1);
        }
        else if(received_bytes == 0) {
            perror("0 bytes were sent, closing ...");
            close(d->socket);
            exit(-1);
        }
        d->buffer[strlen(d->buffer) - 1] = '\0';
        fprintf(stdout, "\r\033[2K[C num] -> %s (bytes %zd)\n", d->buffer, received_bytes);
        fprintf(stdout, "[You] -> ");
        fflush(stdout);
    }

    return NULL;
}

void* send_thread(void* args) {
    thread_data* d = (thread_data*)args;
    while(1) {
        memset(d->buffer, 0, BUFFER_SIZE * sizeof(char));

        fprintf(stdout, "[You] -> ");
        fgets(d->buffer, BUFFER_SIZE * sizeof(char), stdin);
        ssize_t sent_bytes = send(d->socket, d->buffer, strlen(d->buffer), 0);
        if(sent_bytes == -1) {
            perror("Failed to send message!");
            close(d->socket);
            exit(-1);
        }
        d->buffer[strlen(d->buffer) - 1] = '\0';
        fprintf(stdout, "\033[F\033[2K[You] -> %s (bytes %zd)\n", d->buffer, sent_bytes);
    }
    return NULL;
}


int main(int argc, char* argv[]) {
    if(argc != 3) {
        fprintf(stdout, "usage -> <./executable> <IP-address> <PORT number>\n");
        exit(-1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        perror("Failed to create socket");
        exit(-1);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[2]));
    if(inet_pton(AF_INET, argv[1], &address.sin_addr) <= 0) {
        perror("Falied to translate ip address");
        close(sockfd);
        exit(-1);
    }

    if(connect(sockfd, (const struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("Failed to connect to the server!");
        close(sockfd);
        exit(-1);
    }
    fprintf(stdout, "You have succesfuly connected to the server.\n");

    thread_data *send = (thread_data*)malloc(sizeof(thread_data));
    send->buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    send->socket = sockfd;

    thread_data *receive = (thread_data*)malloc(sizeof(thread_data));
    receive->buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    receive->socket = sockfd;

    pthread_t sender;
    pthread_t receiver;

    pthread_create(&sender, NULL, send_thread, send);
    pthread_create(&receiver, NULL, receive_thread, receive);

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    free(send);
    free(receive);    
    close(sockfd);
    return 0;
}