all: bin bin/server bin/client resource/cgi-bin/hello-world
	
bin:
	mkdir bin
	mkdir resource/cgi-bin

bin/server: source/server.c
	gcc source/server.c -o bin/server

bin/client: source/client.c
	gcc source/client.c -o bin/client

clean:
	rm bin/server bin/client
	rmdir bin

resource/cgi-bin/hello-world: resource/cgi-source/hello-world.c
	gcc resource/cgi-source/hello-world.c -o resource/cgi-bin/hello-world -Wall -Werror -lm -fsanitize=address,leak
