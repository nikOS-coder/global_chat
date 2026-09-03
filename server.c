#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

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

    if(listen(serverfd, 10) == -1) {
        perror("Server failed to listen!");
        close(serverfd);
        exit(-1);
    }

    int clientfd = accept(serverfd, (struct sockaddr*)&address, (socklen_t*)&addrsize);
    if(clientfd == -1) {
        perror("Failed to accpet the client!");
        close(serverfd);
        close(clientfd);
        exit(-1);
    }
    fprintf(stdout, "A client has logged in\n");

    close(serverfd);
    close(clientfd);
    return 0;
}