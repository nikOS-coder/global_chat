#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <mongoc/mongoc.h>
#include <bson/bson.h>

#define BUFFER_SIZE 4096
#define USERNAME_SIZE 20
#define PASSWORD_SIZE 50
#define MAX_CLIENTS 10

typedef struct{
    int socket;
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    mongoc_collection_t *collection;
}client_data;

const bson_t* mongo_search_username(mongoc_collection_t*, char*);
char* mongo_check_password(mongoc_collection_t*, char*);
bool insert_credentials(mongoc_collection_t*, const bson_t*);
bool sign_up(client_data*);
bool login(client_data*);

int count = 0;
int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void* client_thread(void* args) {
    client_data *cd = (client_data*)args;

    //setting up customer
    char choice[2] = {0};
    if(recv(cd->socket, choice, sizeof(choice), 0) == -1) {
        perror("Failed to receive choice!");
        close(cd->socket);
    }

    bool success;
    switch(atoi(choice)) {
        case 1: success = sign_up(cd); break;
        case 2: success = login(cd); break;
    }

    if(!success) {
        fprintf(stdout, "Authentication failed for client. Closing connection.\n");
        close(cd->socket);
        free(cd);
        return NULL; 
    }

    //serving the message to clients
    char *buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    while(1) {
        memset(buffer, 0, BUFFER_SIZE * sizeof(char));

        ssize_t received_bytes = recv(cd->socket, buffer, BUFFER_SIZE * sizeof(char), 0);
        if(received_bytes == -1) {
            perror("Failed to receive message!");
            close(cd->socket);
            return NULL;
        }
        else if(received_bytes == 0) {
            perror("0 bytes were sent, closing ...");
            close(cd->socket);
            return NULL;
        }
        buffer[strcspn(buffer, "\n")] = '\0'; 
        fprintf(stdout, "\r\033[2K[%s] -> %s (bytes %zd)\n", cd->username, buffer, received_bytes);

        pthread_mutex_lock(&clients_mutex);
        for(int i = 0; i < count; i++) {
            if(client_sockets[i] != cd->socket) {
                if(send(client_sockets[i], buffer, strlen(buffer), 0) == -1) {
                    fprintf(stdout, "%d customer didnt receive message", i);
                    close(cd->socket);
                    return NULL;
                }

                usleep(100000);

                if(send(client_sockets[i], cd->username, strlen(cd->username), 0) == -1) {
                    fprintf(stdout, "%d customer didnt receive message", i);
                    close(cd->socket);
                    return NULL;
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
    }

    client_data *cd = (client_data*)malloc(sizeof(client_data));

    //mongo
    mongoc_init();
    mongoc_client_t *client = mongoc_client_new("mongodb://localhost:27017");
    if(!client) {
        perror("Failed to create a mongo client");
    }

    mongoc_collection_t *collection = mongoc_client_get_collection(client, "global_chat", "users");
    cd->collection = collection;
    fprintf(stdout, "Succesfully connected to collection users\n");

    //socket
    int serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if(serverfd == -1) {
        perror("Failed to create socket!");
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(atoi(argv[1]));

    int addrsize = sizeof(address);
    if(bind(serverfd, (const struct sockaddr*)&address, addrsize) == -1) {
        perror("Failed to bind server!");
        close(serverfd);
    }

    if(listen(serverfd, MAX_CLIENTS) == -1) {
        perror("Server failed to listen!");
        close(serverfd);
    }

    int clientfd;
    while(1) {
        clientfd = accept(serverfd, (struct sockaddr*)&address, (socklen_t*)&addrsize);
        if(clientfd == -1) {
            perror("Failed to accept the client!");
            close(serverfd);
            close(clientfd);
        }
        pthread_mutex_lock(&clients_mutex);
        if(count < MAX_CLIENTS) {
            pthread_t client_handler;
            cd = (client_data*)malloc(sizeof(client_data));
            cd->socket = clientfd;
            cd->collection = collection;

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

    mongoc_collection_destroy(cd->collection);
    mongoc_client_destroy(client);
    mongoc_cleanup();

    free(cd);
    close(serverfd);
    close(clientfd);
    return 0;
}

const bson_t* mongo_search_username(mongoc_collection_t* collection, char *username) {
    bson_t *filter = BCON_NEW("username", BCON_UTF8(username));

    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, filter, NULL, NULL);
    
    const bson_t *find;
    const bson_t *result = NULL;
    if(mongoc_cursor_next(cursor, &find)) {
        result = bson_copy(find);        
    }

    bson_destroy(filter);
    mongoc_cursor_destroy(cursor);

    return result;
}

char* mongo_check_password(mongoc_collection_t* collection, char *username) {
    const bson_t *doc = mongo_search_username(collection, username);

    bson_iter_t iter;
    char *password = NULL;
    if(bson_iter_init_find(&iter, doc, "password")) {
        password = strdup(bson_iter_utf8(&iter, NULL));
    }

    bson_destroy((bson_t*)doc);
    return password;
}

bool insert_credentials(mongoc_collection_t* collection, const bson_t* doc) {
    bson_error_t error;

    bool success = mongoc_collection_insert_one(collection, doc, NULL, NULL, &error); 
    if(success) {
        fprintf(stdout, "Database was updated\n");
    }
    else{
        fprintf(stderr, "Insertion fail: %s\n", error.message);
    }

    return success;
}

bool sign_up(client_data *cd) {
    char *username = (char*)malloc(USERNAME_SIZE * sizeof(char));
    if(recv(cd->socket, username, USERNAME_SIZE * sizeof(char), 0) == -1) {
        perror("Failed to receive username!");
        close(cd->socket);
        return false;
    }
    username[strcspn(username, "\n")] = '\0';

    if(mongo_search_username(cd->collection, username) != NULL) {
        char *msg1 = "User already exists!\n";
        if(send(cd->socket, msg1, strlen(msg1), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
        }

        return false;
    }
    else {
        char *msg2 = "Valid username\n";
        if(send(cd->socket, msg2, strlen(msg2), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
        }
    }
    
    char *password = (char*)malloc(PASSWORD_SIZE * sizeof(char));
    if(recv(cd->socket, password, PASSWORD_SIZE * sizeof(char), 0) == -1) {
        perror("Failed to receive password!");
        close(cd->socket);
        return false; 
    }
    password[strcspn(password, "\n")] = '\0';

    strcpy(cd->username, username);
    strcpy(cd->password, password);

    bson_t *doc = BCON_NEW(
        "username", BCON_UTF8(username),
        "password", BCON_UTF8(password)
    );

    if(insert_credentials(cd->collection, doc)) {
      fprintf(stdout, "Database got updated\n");
        fprintf(stdout, "Client %s logged in for their first time\n", cd->username);
        
        char *msg3 = "Your credentials are saved\n";
        if(send(cd->socket, msg3, strlen(msg3), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
        }
    }
    else {
        fprintf(stdout, "Failed to update database\n");
      
        char *msg3 = "Failed to save your credentilas\n";
        if(send(cd->socket, msg3, strlen(msg3), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
        } 
        bson_destroy(doc);
        return false;
    }
 
    bson_destroy(doc);

    return true;
}

bool login(client_data *cd) {
    char *username = (char*)malloc(USERNAME_SIZE * sizeof(char));
    if(recv(cd->socket, username, USERNAME_SIZE * sizeof(char), 0) == -1) {
        perror("Failed to receive username!");
        close(cd->socket);
        return false; 
    }
    username[strcspn(username, "\n")] = '\0';

    strcpy(cd->username, username);

    if(mongo_search_username(cd->collection, username) == NULL) {
        char *msg1 = "User does not exist\n";
        if(send(cd->socket, msg1, strlen(msg1), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
        }
        return false;
    }
    else {
        char *msg2 = "Found the username\n";
        if(send(cd->socket, msg2, strlen(msg2), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
        }
    }
    
    int count = 0;
    char *password = (char*)malloc(PASSWORD_SIZE * sizeof(char));
    char *str = (char*)malloc(PASSWORD_SIZE * sizeof(char));
    strcpy(str, mongo_check_password(cd->collection, username));
    do {
        char *msg3 = "Type your password\n";
        if(send(cd->socket, msg3, strlen(msg3), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
        }

        if(recv(cd->socket, password, sizeof(char) * PASSWORD_SIZE, 0) == -1) {
            perror("Failed to receive password!");
            close(cd->socket);
            return false;
        }
        password[strcspn(password, "\r\n")] = '\0';

    }while(strcmp(str, password) != 0 && count++ < 2);

    free(password);
    free(str);

    if(count == 3) {
        char *msg4 = "Seems like you dont know the password, bye-bye\n";
        if(send(cd->socket, msg4, strlen(msg4), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
            return false;
        }
        return false;
    }
    else {
       char *msg4 = "Logging in...\n";
        if(send(cd->socket, msg4, strlen(msg4), 0) == -1) {
            perror("Failed to send message");
            close(cd->socket);
            return false;
        } 
    }

    fprintf(stdout, "Client %s has logged in\n", cd->username);

    return true;
}