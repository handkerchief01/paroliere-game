#
CC = gcc

CFLAGS = -Wall -pthread

SERVER_OBJS = paroliere_srv.o matrice.o server_utilities.o utilities.o trie.o
CLIENT_OBJS = paroliere_cl.o utilities.o

SERVER_TARGET = paroliere_srv
CLIENT_TARGET = paroliere_cl

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS)

paroliere_srv.o: paroliere_srv.c matrice.h server_utilities.h utilities.h trie.h
	$(CC) $(CFLAGS) -c paroliere_srv.c

paroliere_cl.o: paroliere_cl.c utilities.h
	$(CC) $(CFLAGS) -c paroliere_cl.c

matrice.o: matrice.c matrice.h
	$(CC) $(CFLAGS) -c matrice.c

server_utilities.o: server_utilities.c server_utilities.h utilities.h trie.h
	$(CC) $(CFLAGS) -c server_utilities.c

utilities.o: utilities.c utilities.h
	$(CC) $(CFLAGS) -c utilities.c

trie.o: trie.c trie.h
	$(CC) $(CFLAGS) -c trie.c

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET) $(CLIENT_TARGET)
