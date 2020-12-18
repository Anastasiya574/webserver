#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char* argv[]){
	if (argv[1] == NULL)
		execlp("ls" , "ls" , NULL);
	else {
		printf("ip : (127.0.0.1) \n");
		printf("CGI. OPEN FILE\n");
		int fd1 = open(argv[1], O_RDONLY);
		char buf1[5000];
		read(fd1 , buf1, 5000);
		printf("%s\n", buf1);
		if (argv[2]!= NULL) {
			int fd2 = open(argv[2], O_RDONLY);
			char buf2[2000];
			read(fd2 , buf2, 2000);
			printf("%s\n", buf2);
			if (argv[3]!= NULL) {
				int fd3 = open(argv[3], O_RDONLY);
				char buf3[2000];
				read(fd3 , buf3, 2000);
				printf("%s\n", buf3);
			}
		}
	}
 	return 0;
}
