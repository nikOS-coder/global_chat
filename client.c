#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

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

    close(sockfd);
    return 0;
}