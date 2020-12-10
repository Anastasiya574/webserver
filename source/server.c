#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_SETSOCKETOPT,
    ERR_BIND,
    ERR_LISTEN
};

typedef struct {
    char *ext;
    char *mediatype;
} data;

data inf[] = {
    {"txt", "text/plain" },
    {"jpg", "image/jpg" },
    {"png", "image/png" },
    {"html","text/html" },
};

int init_socket(int port);
int interaction_client(int client_socket);
char* get_request(int client_socket);
char* get_path(char* message);
void send_to_client(int client_socket, char* request_path, char* ext);
void do_if_success(int client_socket, int fd, int index);
void do_if_error(int client_socket);

int init_socket(int port) {
    // open socket, return socket descriptor
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fail: open socket");
        _exit(ERR_SOCKET);
    }

    // set socket option
    int socket_option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_option,
              (socklen_t) sizeof(socket_option));
    if (server_socket < 0) {
        perror("Fail: set socket options");
        _exit(ERR_SETSOCKETOPT);
    }

    // set socket address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *) &server_address,
       (socklen_t) sizeof(server_address)) < 0) {
        perror("Fail: bind socket address");
        _exit(ERR_BIND);
    }

    // listen mode start
    if (listen(server_socket, 5) < 0) {
        perror("Fail: bind socket address");
        _exit(ERR_LISTEN);
    }
    return server_socket;
}

char* get_request(int client_socket) {
    char *message = NULL;
    int size = 0;
    char ch, prev = '\0';
    while (read(client_socket, &ch, 1) > 0) {
        size++;
        message = realloc(message, size * sizeof(char));
        message[size-1] = ch;
        if (prev == '\n' && ch == '\r') {
            read(client_socket, &ch, 1);
            size++;
            message = realloc(message, size * sizeof(char));
            message[size-1] = ch;
            break;
        }
        prev = ch;
    }
    size++;
    message = realloc(message, size * sizeof(char));
    message[size-1] = '\0';
    return message;
}

int interaction_client(int client_socket) {
    while(1) {
        char *message = NULL;
        message = get_request(client_socket);
        if (message && strlen(message) != 0) {
            puts(message);
            char *request_path = get_path(message);
            char *ext = strchr(request_path, '.');
            if (ext != NULL) {
                send_to_client(client_socket, request_path, ext + 1); 
            } //else {
           // }
            free(message);
        } else {
            free(message);
            break;
        }
    }
    return 0;
}

void send_to_client(int client_socket, char* request_path, char* ext) {
    int i = 0;
    while (inf[i].ext) {
        if (strcmp(inf[i].ext, ext) != 0) {
            i++;
        } else {
            break;
        }
    }
    if (!inf[i].ext) {
        puts("NOT FOUND");
        do_if_error(client_socket);
        return;
    }
    int fd =  open(request_path + 1, O_RDONLY);
    puts(request_path + 1);
    if (fd < 0) {
        puts("NOT FOUND");
        do_if_error(client_socket);
        return;
    } else {
        do_if_success(client_socket, fd, i);
    }

    close(fd);
}

void do_if_success(int client_socket, int fd, int index) {
    char h1[] = "HTTP/1.1 404\r\ncontent-type: text/html\r\ncontent-length: %d\r\n\r\n";
    char h2[] = "<!DOCTYPE html><html><h1>";
    char h3[] = "</h1></html>";
    int size = 0; 
    char* buf = NULL;
    char message[100];
    char ch;
    while (read(fd, &ch, 1) > 0) {
        size++;
        buf = realloc(buf, size * sizeof(char));
        buf[size-1] = ch;
    }
    size++;
    buf = realloc(buf, size * sizeof(char));
    buf[size-1] = '\0';
    sprintf(message, h1, inf[index].mediatype, strlen(h2) + strlen(buf) + strlen(h3));
    write(client_socket, message, strlen(message));
    write(client_socket, h2, strlen(h2));
    write(client_socket, buf, strlen(buf));
    write(client_socket, h3, strlen(h3));
    free(buf);
}

void do_if_error(int client_socket) {
    char h1[] = "HTTP/1.1 404\r\n\r\n";
    write(client_socket, h1, strlen(h1));
}

char* get_path(char* message) {
    char *req;
    req = strtok(message, " \r\n");
    req = strtok(NULL, " \r\n");
    puts(req);
    return req;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        puts("Incorrect args.");
        puts("./server <port> <number_of_clients>");
        puts("Example:");
        puts("./server 8080 1");
        return ERR_INCORRECT_ARGS;
    }
    int port = atoi(argv[1]);
    int server_socket = init_socket(port);
    puts("Wait for connection");
    struct sockaddr_in client_address;
    socklen_t size = sizeof(client_address);
    int clients = atoi(argv[2]);
    int *client_socket = malloc(clients * sizeof(int));
    for (int i = 0; i < clients; i++) {
        client_socket[i] = accept(server_socket, 
                           (struct sockaddr *) &client_address,
                           &size);
        printf("Client %d connected:\n", i+1);
        if (fork() == 0) {
            interaction_client(client_socket[i]);
            exit(0);
        }
        close(client_socket[i]);
    }
    for (int i = 0; i < clients; i++) {
        wait(NULL);
    }
    free(client_socket);
    return OK;
}
