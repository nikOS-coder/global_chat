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

typedef struct {
    char *buffer;
    int socket;
}thread_data;

void* receive_thread(void* args) {
    int socket = *(int *)args;

    while(1) {
        char *buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
        memset(buffer, 0, BUFFER_SIZE * sizeof(char));

        ssize_t received_bytes = recv(socket, buffer, BUFFER_SIZE * sizeof(char), 0);
        if(received_bytes == -1) {
            perror("Failed to receive message!");
            close(socket);
            exit(-1);
        }
        else if(received_bytes == 0) {
            perror("0 bytes were sent, closing ...");
            close(socket);
            exit(-1);
        }

        char *username = (char*)malloc(USERNAME_SIZE * sizeof(char));
        received_bytes = recv(socket, username, USERNAME_SIZE * sizeof(char), 0);
        if(received_bytes == -1) {
            perror("Failed to receive message!");
            close(socket);
            exit(-1);
        }
        else if(received_bytes == 0) {
            perror("0 bytes were sent, closing ...");
            close(socket);
            exit(-1);
        }   

        buffer[strcspn(buffer, "\n")] = '\0'; 
        username[strcspn(username, "\n")] = '\0'; 

        fprintf(stdout, "\r\033[2K[%s] -> %s (bytes %zd)\n", username, buffer, received_bytes);
        fprintf(stdout, "[You] -> ");
        fflush(stdout);
    }

    return NULL;
}

void* send_thread(void* args) {
    int socket = *(int*)args;

    while(1) {
        char *buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
        memset(buffer, 0, BUFFER_SIZE * sizeof(char));

        fprintf(stdout, "[You] -> ");
        fgets(buffer, BUFFER_SIZE * sizeof(char), stdin);
        ssize_t sent_bytes = send(socket, buffer, strlen(buffer), 0);
        if(sent_bytes == -1) {
            perror("Failed to send message!");
            close(socket);
            exit(-1);
        }
        buffer[strcspn(buffer, "\n")] = '\0'; 
        fprintf(stdout, "\033[F\033[2K[You] -> %s (bytes %zd)\n", buffer, sent_bytes);
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
    char* username = (char*)malloc(USERNAME_SIZE * sizeof(char));
    char* password = (char*)malloc(PASSWORD_SIZE * sizeof(char));

    fprintf(stdout, "Give your username: ");
    fgets(username, USERNAME_SIZE * sizeof(char), stdin);
    username[strcspn(username, "\n")] = '\0'; 
    
    fprintf(stdout, "Give your password: ");
    fgets(password, PASSWORD_SIZE * sizeof(char), stdin);
    password[strcspn(password, "\n")] = '\0'; 

    fprintf(stdout, "You have succesfuly connected to the server.\n");

    if(send(sockfd, username, strlen(username), 0) == -1) {
        perror("Failed to send username!");
        close(sockfd);
        exit(-1);
    }

    usleep(100000);

    if(send(sockfd, password, strlen(password), 0) == -1) {
        perror("Failed to send password!");
        close(sockfd);
        exit(-1);
    }

    pthread_t sender;
    pthread_t receiver;

    pthread_create(&sender, NULL, send_thread, &sockfd);
    pthread_create(&receiver, NULL, receive_thread, &sockfd);

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);

    free(username);
    free(password);   
    close(sockfd);
    return 0;
}