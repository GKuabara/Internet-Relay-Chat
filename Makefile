CC=gcc
CFLAGS = -Wall
LDFLAGS = 
OBJFILES = server.o client.o utils.o socket.o
OBJFILES_SERVER = server.o utils.o socket.o
OBJFILES_CLIENT = client.o utils.o socket.o
OBJFILES_UTILS = utils.o socket.o
TARGET = server client
TARGET_SERVER = server
TARGET_CLIENT = client
TARGET_UTILS = utils socket

all: $(TARGET)

$(TARGET_SERVER): $(OBJFILES_SERVER)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJFILES_SERVER)

$(TARGET_CLIENT): $(OBJFILES_CLIENT)
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJFILES_CLIENT)

clean:
	rm -f $(OBJFILES) $(TARGET)

# old ----------

# all: server client

# server:
# 	$(CC) server.c -o server

# client:
# 	$(CC) client.c -o client

# run-server:
# 	@./server

# run-client:
# 	@./client

# clean:
# 	rm client server

# zip:
# 	zip -r first-part.zip Makefile server.c client.c README.md