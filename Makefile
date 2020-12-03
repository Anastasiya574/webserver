all: bin bin/server bin/client resource
	
bin:
	mkdir bin

bin/server: server.c
	gcc server.c -o bin/server -Wall -Werror -lm -fsanitize=address,leak

bin/client: client.c
	gcc client.c -o bin/client -Wall -Werror -lm -fsanitize=address,leak

clean:
	rm bin/server bin/client
	rmdir bin

resource: resource/cgi-source/hello-world.c
	gcc resource/cgi-source/hello-world.c -o resource/cgi-bin/hello-world -Wall -Werror -lm -fsanitize=address,leak
