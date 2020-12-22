#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[]) {
    printf("content-length: ");
    long length = 0;
    for (int i = 0 ; i < argc ; i++) {
        length += strlen(argv[i]);
    }
    length += argc;
    printf("%ld\r\n\r\n", length);
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\r\n");
}
