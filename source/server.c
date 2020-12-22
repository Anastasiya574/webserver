#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sendfile.h>

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_SETSOCKETOPT,
    ERR_BIND,
    ERR_LISTEN
};

enum types {
    TEXT,
    BINARY,
    MULTIMEDIA
};

char HEADER[] = "HTTP/1.1 ";
char CONTENT_LENGTH[] = "content-length: ";
char CONTENT_TYPE[] = "content-type: ";
char END[] = "\r\n";
char bad_request[] = "404\n";
char good_request[] = "200\n";
char TYPE_VALUE[1][100] = {"text/html\r\n"};

int init_socket(int port);
char *get_word(int client_socket);
char *get_filename(int client_socket);
int get_file_type(char *filename);
char *get_file_size(char *filename);
char *get_file_content(char *filename, int fd);
char *push_back(char *dest, char *src);
char *get_value(char *filename, int *i);
char **do_query(char *filename);
int free_values(char **values);
void print_values(char **values);
void send_text(int client_socket, char *filename);
void send_bin(int client_socket, char *filename);
void send_multimedia(int client_socket, char *filename);
void run(int client_socket);

int main(int argc, char** argv) {
    struct sockaddr_in client_address;
    socklen_t size = sizeof client_address;
    if (argc != 3) {
        puts("Incorrect args.");
        puts("./server <port> <client_number>");
        puts("Example:");
        puts("./server 8080 1");
        return ERR_INCORRECT_ARGS;
    }
    int port = atoi(argv[1]);
    int client_num = atoi(argv[2]);
    int server_socket = init_socket(port);
    int client_socket;
    for (int i = 0 ; i < client_num ; i++) {
        while (1) {
            puts("wait for connection");
            client_socket = accept(server_socket,
                            (struct sockaddr *) &client_address,
                            &size);
            printf("connected: %s %d\n", inet_ntoa(client_address.sin_addr),
                            ntohs(client_address.sin_port));
            int pid = fork();
            if (pid == 0) {
                run(client_socket);
            } else {
                wait(NULL);
            }
            close(client_socket);
        }
    }
    return OK;
}

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

char *get_word(int client_socket) {
    char *word = NULL;
    char ch;
    int size = 0;
    for (read(client_socket, &ch, 1);
                ch != ' ' && ch != '\n' && ch != '\0';
                read(client_socket, &ch, 1)) {
        size++;
        word = realloc(word, (size + 1) * sizeof(char));
        word[size - 1] = ch;
    }
    if (word)
        word[size] = '\0';
    return word;
}

char *get_filename(int client_socket) {
    char *filename;
    char *message = get_word(client_socket);
    if (strcmp("GET", message) ) { 
        perror("incorrect query");
    }
    free(message);   
    filename = get_word(client_socket);
    message = get_word(client_socket);
    free(message);
    message = get_word(client_socket);
    if (strcmp("Host:", message)) {
        perror("incorrect query");
    }
    free(message);
    message = get_word(client_socket);
    free(message);
    return filename;
}

int get_file_type(char *filename) {
    if (!filename ) {
        return -1;
    }
    int i;
    for (i = 0; filename[i]; i++) {
        if (filename[i] == '.') {
            if (!strcmp(filename + i, ".png") ||
                !strcmp(filename + i, ".jpg")) {
                printf("multimedia\n");
                return MULTIMEDIA;
            }
            if (!strcmp(filename + i, ".html") || !strcmp(filename + i, ".txt")) {
                printf("html/txt\n");
                return TEXT;
            }
        }
    }
    return BINARY;
}

char *push_back(char *dest, char *src) {
    dest = realloc(dest, sizeof(char) * (strlen(dest) + strlen(src) - 1));
    strcat(dest, src);
    return dest;
}

char *get_file_size(char *filename) {
    char *answ = NULL;
    struct stat stats;
    if (stat(filename, &stats) != 0) {
        perror("stat failed");
    }
    long size = stats.st_size;
    int length = 0;
    for (int size_copy = size; size_copy; length++) {
        size_copy /= 10;
    }
    answ = malloc(length);
    sprintf(answ, "%ld", size);
    answ = push_back(answ, END);
    return answ;
}

char *get_file_content(char *filename, int fd) {
    struct stat stats;
    stat(filename, &stats);
    char *buff = malloc(stats.st_size);
    read(fd, buff, stats.st_size);
    return buff;
}

char *get_value(char *filename, int *i) {
    char *answ = NULL;
    int size = 0;
    for ( ; (filename[*i] != '\0') && (filename[*i] != '=') && (filename[*i] != '&') ; (*i)++ ) {
        size++;
        answ = realloc(answ, size + 1);
        answ[size - 1] = filename[*i];
    }
    answ[size] = '\0';
    return answ;
}

char **do_query(char *filename) {
    int i = 0;
    for ( ; filename[i] != '\0' && filename[i] != '?'; i++) {
    }
    int filesize = strlen(filename);
    if (filename[i] == '?') {
        filename[i] = '\0';
    }
    i++;
    int size = 1;
    char **answ = malloc(sizeof(char *) * 2);
    answ[0] = malloc(strlen(filename));
    strcpy(answ[0], filename);
    for ( ; i < filesize ; ) {
        size++;
        answ = realloc(answ, sizeof(char *) * (size + 1));
        answ[size - 1] = get_value(filename, &i);
        i++;
    }
    answ[size] = NULL;
    return answ;
}

int free_values(char **values) {
    for (int i = 0; values[i]; i++)
        free(values[i]);
    free(values);
    return 1;
}

void print_values(char **values) {
    for (int i = 0; values[i]; i++)
        printf("\n%s\n", values[i]);
    return;
}

void send_text(int client_socket, char *filename) {
    int fd = open(filename, O_RDONLY, 0);
    char *answ = NULL;
    if (fd < 0) {
        write(client_socket, HEADER, strlen(HEADER));
        write(client_socket, bad_request, strlen(bad_request));
        close(fd);
        return;
    }
    char *size_string = get_file_size(filename);
    char *content = get_file_content(filename, fd);
    answ = malloc(strlen(HEADER));
    strcpy(answ, HEADER);
    write(client_socket, good_request, strlen(good_request));
    write(client_socket, CONTENT_TYPE, strlen(CONTENT_TYPE));
    write(client_socket, TYPE_VALUE[0], strlen(TYPE_VALUE[0]));
    write(client_socket, CONTENT_LENGTH, strlen(CONTENT_LENGTH));
    write(client_socket, size_string, strlen(size_string));
    write(client_socket, END, strlen(END));
    write(client_socket, content, strlen(content));
    free(size_string);
    free(content);
    close(fd);
    return;
}

void send_bin(int client_socket, char *filename) {
    char **values = do_query(filename);
    print_values(values);
    int fd = (filename, O_RDONLY, 0);
    if (fd < 0) {
        write(client_socket, HEADER, strlen(HEADER));
        write(client_socket, bad_request, strlen(bad_request));
        free_values(values);
        return;
    }
    write(client_socket, HEADER, strlen(HEADER));
    write(client_socket, good_request, strlen(good_request));
    write(client_socket, CONTENT_TYPE, strlen(CONTENT_TYPE));
    write(client_socket, TYPE_VALUE[0], strlen(TYPE_VALUE[0]));
    int pid = fork();
    if (pid == 0) {
        dup2(client_socket, 1);
        execv(filename, values);
        exit(0);
    } else {
        wait(NULL);
    }
    free_values(values);
    close(fd);
    return;
}

void send_multimedia(int client_socket, char *filename) {
    int fd = open(filename, O_RDONLY, 0);
    if (fd < 0) {
        write(client_socket, HEADER, strlen(HEADER));
        write(client_socket, bad_request, strlen(bad_request));
        close(fd);
        return;
    }
    write(client_socket, HEADER, strlen(HEADER));
    write(client_socket, good_request, strlen(good_request));
    write(client_socket, CONTENT_TYPE, strlen(CONTENT_TYPE));
    write(client_socket, "image/", strlen("image/"));
    int i = 0;
    for ( ; filename[i] != '.' ;i++) {
    }
    write(client_socket, filename + i + 1, strlen(filename + i + 1));
    write(client_socket, END, strlen(END));
    write(client_socket, CONTENT_LENGTH, strlen(CONTENT_LENGTH));
    write(client_socket, get_file_size(filename), strlen(get_file_size(filename)));
    write(client_socket, END, strlen(END));
    close(fd);
    return;
}

void run(int client_socket) {
    char *filename = get_filename(client_socket);
    int file_type = get_file_type(filename);
    if (file_type == TEXT) {
        send_text(client_socket, (filename + 1));
        free(filename);
        return;
    }
    if (file_type == MULTIMEDIA) {
        send_multimedia(client_socket, (filename + 1));
        int fd = open(filename + 1, O_RDONLY, 0);
        while(sendfile(client_socket, fd, 0, 1)) {
        }
        free(filename);
        return;
    }
    if (file_type == BINARY) {
        send_bin(client_socket, filename + 1);
        free(filename);
        return;
    }
    return;
}
