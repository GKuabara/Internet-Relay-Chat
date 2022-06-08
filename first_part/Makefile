CC=gcc

all: server client

server:
	$(CC) server.c -o server

client:
	$(CC) client.c -o client

run-server:
	@./server

run-client:
	@./client

clean:
	rm client server

zip:
	zip -r first-part.zip Makefile server.c client.c README.md