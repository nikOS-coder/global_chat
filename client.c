#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define BUFFER_SIZE 4096
#define USERNAME_SIZE 20
#define PASSWORD_SIZE 50

void sign_upGUI(int);
void loginGUI();

void* receive_thread(void* args) {
    int socket = *(int *)args;

    while(1) {
        char *buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
        memset(buffer, 0, BUFFER_SIZE * sizeof(char));

        ssize_t received_bytes = recv(socket, buffer, BUFFER_SIZE * sizeof(char), 0);
        if(received_bytes == -1) {
            perror("Failed to receive message!");
            close(socket);
            exit(EXIT_FAILURE);
        }
        else if(received_bytes == 0) {
            perror("0 bytes were sent, closing ...");
            close(socket);
            exit(EXIT_FAILURE);
        }

        char *username = (char*)malloc(USERNAME_SIZE * sizeof(char));
        received_bytes = recv(socket, username, USERNAME_SIZE * sizeof(char), 0);
        if(received_bytes == -1) {
            perror("Failed to receive message!");
            close(socket);
            exit(EXIT_FAILURE);
        }
        else if(received_bytes == 0) {
            perror("0 bytes were sent, closing ...");
            close(socket);
            exit(EXIT_FAILURE);
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
        fflush(stdout);
        fgets(buffer, BUFFER_SIZE * sizeof(char), stdin);
        ssize_t sent_bytes = send(socket, buffer, strlen(buffer), 0);
        if(sent_bytes == -1) {
            perror("Failed to send message!");
            close(socket);
            exit(EXIT_FAILURE);
        }
        buffer[strcspn(buffer, "\n")] = '\0'; 
        fprintf(stdout, "\033[F\033[2K[You] -> %s (bytes %zd)\n", buffer, sent_bytes);
    }
    return NULL;
}


int main(int argc, char* argv[]) {
    if(argc != 3) {
        fprintf(stdout, "usage -> <./executable> <IP-address> <PORT number>\n");
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[2]));
    if(inet_pton(AF_INET, argv[1], &address.sin_addr) <= 0) {
        perror("Falied to translate ip address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if(connect(sockfd, (const struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("Failed to connect to the server!");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout, "Do you wish to login or sign up?\n");
    fprintf(stdout, "[1]... sign-up\n");
    fprintf(stdout, "[2]... login\n");
    char choice_buf[10];
    fgets(choice_buf, 10 * sizeof(char), stdin);
    int choice = atoi(choice_buf);


    switch(choice) {
        case 1: sign_upGUI(sockfd); break;
        case 2: loginGUI(sockfd); break;
        default: fprintf(stdout, "Not a valid choice, closing...\n"); exit(EXIT_FAILURE);
    }

    fprintf(stdout, "You have succesfully logged in\n");

    pthread_t sender;
    pthread_t receiver;

    pthread_create(&sender, NULL, send_thread, &sockfd);
    pthread_create(&receiver, NULL, receive_thread, &sockfd);

    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);
  
    close(sockfd);
    return 0;
}

void sign_upGUI(int socket) {
    char *choice = "1";
    if(send(socket, choice, strlen(choice), 0) == -1) {
        perror("Failed to send choice to server!");
        close(socket);
        exit(EXIT_FAILURE);
    }

    char *username = (char*)malloc(USERNAME_SIZE * sizeof(char));
    fprintf(stdout, "Type your username: ");
    fgets(username, USERNAME_SIZE * sizeof(char), stdin);

    if(send(socket, username, strlen(username), 0) == -1) {
        perror("Failed to send username!");
        close(socket);
        exit(EXIT_FAILURE); 
    }

    char *msg1 = (char*)malloc(256 * sizeof(char));
    if(recv(socket, msg1, 256 * sizeof(char), 0) == -1) {
        perror("failed to receive message!");
        close(socket);
        exit(EXIT_FAILURE);
    } 
    msg1[strcspn(msg1, "\n")] = '\0';
    fprintf(stdout, "%s\n", msg1);
    if(strcmp(msg1, "User already exists!") == 0) {
        close(socket);
        exit(EXIT_FAILURE);
    }

    char *password = (char*)malloc(PASSWORD_SIZE * sizeof(char));
    fprintf(stdout, "Give a strong password: ");
    fgets(password, PASSWORD_SIZE * sizeof(char), stdin);

    if(send(socket, password, strlen(password), 0) == -1) {
        perror("Failed to send password!");
        close(socket);
        exit(EXIT_FAILURE); 
    }

    char *msg2 = (char*)malloc(256 * sizeof(char));
    if(recv(socket, msg2, 256 * sizeof(char), 0) == -1) {
        perror("failed to receive message!");
        close(socket);
        exit(EXIT_FAILURE);
    } 
    msg2[strcspn(msg2, "\n")] = '\0';
    fprintf(stdout, "%s\n", msg2);
}

void loginGUI(int socket) {
    char *choice = "2";
    if(send(socket, choice, strlen(choice), 0) == -1) {
        perror("Failed to send choice to server!");
        close(socket);
        exit(EXIT_FAILURE);
    }

    char *username = (char*)malloc(USERNAME_SIZE * sizeof(char));
    fprintf(stdout, "Type your username: ");
    fgets(username, USERNAME_SIZE * sizeof(char), stdin);

    if(send(socket, username, strlen(username), 0) == -1) {
        perror("Failed to send username!");
        close(socket);
        exit(EXIT_FAILURE); 
    }

    char *msg1 = (char*)malloc(256 * sizeof(char));
    if(recv(socket, msg1, 256 * sizeof(char), 0) == -1) {
        perror("failed to receive message!");
        close(socket);
        exit(EXIT_FAILURE);
    } 
    msg1[strcspn(msg1, "\n")] = '\0';
    fprintf(stdout, "%s\n", msg1);
    if(strcmp(msg1, "User does not exist") == 0) {
        close(socket);
        exit(EXIT_FAILURE);
    }

    char *password = (char*)malloc(PASSWORD_SIZE * sizeof(char));
    fprintf(stdout, "You have three tries to type your password!\n");

    char *msg2 = (char*)malloc(256 * sizeof(char));
    do {
        if(recv(socket, msg2, 256 * sizeof(char), 0) == -1) {
            perror("failed to receive message!");
            close(socket);
            exit(EXIT_FAILURE);
        } 
        msg2[strcspn(msg2, "\n")] = '\0';
        fprintf(stdout, "%s\n", msg2);

        fgets(password, PASSWORD_SIZE * sizeof(char), stdin);

        if(send(socket, password, strlen(password), 0) == -1) {
            perror("Failed to send password!");
            close(socket);
            exit(EXIT_FAILURE); 
        }
    }while(strcmp(msg2, "Type your password") == 0);
}