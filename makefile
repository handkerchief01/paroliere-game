# Definisce il compilatore
CC = gcc

# Definisce le opzioni del compilatore
CFLAGS = -Wall -pthread

# Definisce i file oggetto
OBJS = paroliere_server.o paroliere_client.o

# Definisce i file eseguibili
TARGETS = paroliere_server paroliere_client

# Regola predefinita, compila tutti i target
all: $(TARGETS)

# Regola per compilare il paroliere_server
paroliere_server: paroliere_server.o
	$(CC) $(CFLAGS) -o paroliere_server paroliere_server.o

# Regola per compilare il paroliere_client
paroliere_client: paroliere_client.o
	$(CC) $(CFLAGS) -o paroliere_client paroliere_client.o

# Regola per compilare il file oggetto paroliere_server.o
paroliere_server.o: paroliere_server.c
	$(CC) $(CFLAGS) -c paroliere_server.c

# Regola per compilare il file oggetto paroliere_client.o
paroliere_client.o: paroliere_client.c
	$(CC) $(CFLAGS) -c paroliere_client.c

# Regola per pulire i file oggetto e gli eseguibili
clean:
	rm -f $(OBJS) $(TARGETS)
