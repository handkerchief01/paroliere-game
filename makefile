# Definisce il compilatore
CC = gcc

# Definisce le opzioni del compilatore
CFLAGS = -Wall -pthread

# Definisce i file oggetto
SERVER_OBJS = paroliere_server.o matrice.o
CLIENT_OBJS = paroliere_client.o matrice.o

# Definisce i file eseguibili
SERVER_TARGET = paroliere_server
CLIENT_TARGET = paroliere_client

# Regola predefinita, compila tutti i target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Regola per compilare il server
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS)

# Regola per compilare il client
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS)

# Regola per compilare il file oggetto server.o
paroliere_server.o: paroliere_server.c matrice.h
	$(CC) $(CFLAGS) -c paroliere_server.c

# Regola per compilare il file oggetto client.o
paroliere_client.o: paroliere_client.c matrice.h
	$(CC) $(CFLAGS) -c paroliere_client.c

# Regola per compilare il file oggetto matrice.o
matrice.o: matrice.c matrice.h
	$(CC) $(CFLAGS) -c matrice.c

# Regola per pulire i file oggetto e gli eseguibili
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET) $(CLIENT_TARGET)
