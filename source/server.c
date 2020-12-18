#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include<time.h>
#include <sys/wait.h>

#define MESSAGE 10000
#define CONNMAX 1000
#define BYTES 1024

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_SETSOCKETOPT,
    ERR_BIND,
    ERR_LISTEN
};

char *ROOT;
int listenfd, client_socket[CONNMAX];
void error(char *);
void freelist(char **);
char* get_binary(char * );
char* naming2(char *);
char** run(char *);
void init_socket(char *);
void respond(int);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        puts("Incorrect args.");
        puts("./server <port>");
        puts("Example:");
        puts("./server 5000");
        return ERR_INCORRECT_ARGS;
    }
	struct sockaddr_in client_address;
	socklen_t addrlen;
        char *port = argv[1];
        //sprintf(port, "%d", pt);
        //strcpy(port, "5000");
	ROOT = getenv("PWD");
        int slot=0;
	int i;
	for (i=0; i<CONNMAX; i++)
	client_socket[i]=-1;
	init_socket(port);
	while (1)
	{
		addrlen = sizeof(client_address);
		client_socket[slot] = accept (listenfd, (struct sockaddr *) &client_address, &addrlen);
		if (client_socket[slot] < 0)
			error ("accept() error");
		else {
			if (fork() == 0) {
				respond(slot);
				exit(0);
			}
		}
		while (client_socket[slot]!=-1) slot = (slot+1)%CONNMAX;
	}
	return OK;
}

void init_socket(char *port) {
	struct addrinfo hints, *res, *p;
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0) {
		perror ("getaddrinfo() error");
		exit(1);
	}
	// socket and bind
	for (p = res; p!=NULL; p=p->ai_next) {
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) continue;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p==NULL) {
		perror ("socket() or bind()");
		exit(1);
	}
	freeaddrinfo(res);
	if ( listen (listenfd, 100000) != 0 ) {
		perror("listen() error");
		exit(1);
	}
}
/*
void init_socket(int port) {
    int server_socket, socket_option = 1;
    struct sockaddr_in server_address;
    //open socket, return socket descriptor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fail: open socket");
        exit(ERR_SOCKET);
    }
    //set socket option
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_option, (socklen_t) sizeof socket_option);
    if (server_socket < 0) {
        perror("Fail: set socket options");
        exit(ERR_SETSOCKETOPT);
    }
    //set socket address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *) &server_address, (socklen_t) sizeof server_address) < 0) {
        perror("Fail: bind socket address");
        exit(ERR_BIND);
    }
    //listen mode start
    if (listen(server_socket, 5) < 0) {
        perror("Fail: bind socket address");
        exit(ERR_LISTEN);
    }
    return;
} */

char** run(char *name) {
    char tmp[] = "/cgi-bin/prog?";
    if (strlen(tmp) >= strlen(name)) {
        return 0;
    }

    char *substr = malloc((strlen(tmp) + 1) * sizeof(char));

    for (int i = 0; i < strlen(tmp); i++) {
        substr[i] = name[i];
    }
    substr[strlen(tmp)] = '\0';

    if (!strcmp(substr, tmp)) {
        char **fnames = malloc(sizeof(char*));
        char bin[30];
        bin[0] = '.';
        int cnt1;
        int cnt2;
        for (cnt1 = 1, cnt2 = 0; name[cnt2]!= '?'; cnt1++, cnt2++)
          bin[cnt1] = name[cnt2];
        bin[cnt1] = '\0';
        fnames[0] = malloc((strlen(bin) + 1) * sizeof(char));
        for (int cnt = 0; cnt < strlen(bin); cnt++) {
          fnames[0][cnt] = bin[cnt];
        }
        fnames[0][strlen(bin)] = '\0';
        int words = 1;
        char buf[100];
        int bufSize = 0;
        int i = strlen(tmp);
        char cgi[] = "cgi-bin/";

        if (strlen(name) > strlen(tmp)) {
            while (name[i] != '\0') {
                if (name[i] != '?') {
                    buf[bufSize] = name[i];
                    bufSize++;
                } else {
                    buf[bufSize] = '\0';
                    words++;
                    fnames = realloc(fnames, words * sizeof(char*));
                    int wordSize = bufSize + strlen(cgi);
                    char *word = malloc((wordSize + 1) * sizeof(char));
                    int j = 0;
                    for (; j < strlen(cgi); j++) {
                        word[j] = cgi[j];
                    }
                    for (int k = 0; j < wordSize; j++,k++) {
                        word[j] = buf[k];
                    }
                    word[wordSize] = '\0';
                    fnames[words-1] = word;

                    bufSize = 0;
                }
                i++;
            }
            buf[bufSize] = '\0';
            words++;
            fnames = realloc(fnames, words * sizeof(char*));
            int wordSize = bufSize + strlen(cgi);
            char *word = malloc((wordSize + 1) * sizeof(char));
            int j = 0;
            for (; j < strlen(cgi); j++) {
                word[j] = cgi[j];
            }
            for (int k = 0; j < wordSize; j++,k++) {
                word[j] = buf[k];
            }
            word[wordSize] = '\0';
            fnames[words-1] = word;

        }
        words++;

        fnames = realloc(fnames, words * sizeof(char*));
        fnames[words - 1] = NULL;
        free(substr);
        return fnames;
    } else {
        free(substr);
        return NULL;
    }
}

void cleaner(char **fname) {
    if (fname != NULL) {
        int i = 0;
        while (fname[i] != NULL) {
            free(fname[i]);
            i++;
        }
        free(fname);
    }
}

char* naming2(char *pname) {
    char *fname = malloc(100 * sizeof(char));
    fname[0] = '.';
    int i;
    int j;
    for (i = 0, j = 1; pname[i]!= '\n'; i++, j++) {
        fname[j] = pname[i];
    }
    return fname;
}

char *get_binary(char *name) {
    char *fname = malloc(20);
    fname[0] = '.';
    int i;
    int j;
    for (i = 1, j = 0; name[j]!= '?'; i++, j++)
        fname[i] = name[j];
    fname[i] = '\0';
    return fname;
}

void respond(int n) {
    char mesg[MESSAGE], *reqline[3], data_to_send[BYTES], path[MESSAGE];
    int rcvd, fd, bytes_read;
    memset((void*)mesg, (int)'\0', MESSAGE);
    rcvd=recv(client_socket[n], mesg, MESSAGE, 0);
    if (rcvd < 0)
        fprintf(stderr,("recv() error\n"));
    else if (rcvd == 0)
        fprintf(stderr,"Client connected\n");
    else {
        printf("msg : %s", mesg);
        reqline[0] = strtok (mesg, " \t\n");
        if ( strncmp(reqline[0], "GET\0", 4) == 0) {
            reqline[1] = strtok (NULL, " \t");
            reqline[2] = strtok (NULL, " \t\n");
            if ( strncmp( reqline[2], "HTTP/1.0", 8) !=0 && strncmp( reqline[2], "HTTP/1.1", 8) !=0 )
                write(client_socket[n], "HTTP/1.0 404 Bad Request\n", 25);
            else {
                if (strncmp(reqline[1], "/\0", 2) == 0)
                    reqline[1] = "/index.html";
            strcpy(path, ROOT);
            strcpy(&path[strlen(ROOT)], reqline[1]);
                if (strncmp(reqline[1] , "/cgi-bin/", 9) == 0) {
                    pid_t pid;
                    char **name, *name2, *name3;
                if ((pid = fork()) == 0) {
                    name2 = naming2(reqline[1]);
                    name = run(reqline[1]);
                    fd = open("cgi.txt", O_WRONLY|O_TRUNC|O_CREAT,
                                          S_IREAD|S_IWRITE);
                    dup2(fd, 1);
                if (name == NULL) {
                    execlp(name2 , name2 ,NULL);
                }
                else {
                    name3 = get_binary(reqline[1]);
                    execvp(name3 , name);
                }
          }
          waitpid(pid, 0, 0);
          fd = open("cgi.txt", O_RDONLY);
          send(client_socket[n], "HTTP/1.0 200 OK\n\n", 17, 0);
					while ((bytes_read = read(fd, data_to_send, BYTES)) > 0)
					     write (client_socket[n], data_to_send, bytes_read);
          close(fd);
          free(name3);
          cleaner(name);
          free(name2);
        }
				else if ((fd = open(path, O_RDONLY)) != -1)
				{
					send(client_socket[n], "HTTP/1.0 200 OK\n\n", 17, 0);
					while ((bytes_read = read(fd, data_to_send, BYTES)) > 0)
					write (client_socket[n], data_to_send, bytes_read);
			}
		else write(client_socket[n], "HTTP/1.1 404\n", 23);
			}
		}
	}
	shutdown (client_socket[n], SHUT_RDWR);
	close(client_socket[n]);
	client_socket[n]=-1;
}
