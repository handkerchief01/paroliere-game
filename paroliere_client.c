#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Definizione dei tipi di messaggio
#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'

// Funzione per inviare un messaggio
void send_message(int sock, char type, const char *data)
{
  unsigned int length = strlen(data);

  // Invia il tipo di messaggio
  write(sock, &type, sizeof(type));
  // Invia la lunghezza del messaggio
  write(sock, &length, sizeof(length));
  // Invia i dati del messaggio
  write(sock, data, length);
}

// Funzione per ricevere un messaggio
void receive_message(int sock, char *type, unsigned int *length, char *data)
{
  // Riceve il tipo di messaggio
  read(sock, type, sizeof(*type));
  // Riceve la lunghezza del messaggio
  read(sock, length, sizeof(*length));
  // Riceve i dati del messaggio
  read(sock, data, *length);
  data[*length] = '\0'; // Aggiunge il terminatore di stringa
}

int main(int argc, char *argv[])
{
  int sock;
  struct sockaddr_in server_address;
  int port = atoi(argv[1]); // Ottiene la porta del server dalla riga di comando

  // Crea il socket del client
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Errore nella creazione del socket");
    exit(EXIT_FAILURE);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  // Converte l'indirizzo IP in formato binario e lo assegna a server_address.sin_addr
  if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
  {
    perror("Indirizzo non valido/Indirizzo non supportato");
    exit(EXIT_FAILURE);
  }

  // Effettua la connessione al server
  if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
  {
    perror("Errore nella connessione");
    exit(EXIT_FAILURE);
  }

  printf("Connesso al server\n");

  // Esempio di invio di un messaggio al server
  send_message(sock, MSG_REGISTRA_UTENTE, "NomeUtente");

  // Esempio di ricezione di un messaggio dal server
  char type;
  unsigned int length;
  char data[1024];
  receive_message(sock, &type, &length, data);

  printf("Ricevuto dal server: Type=%c, Length=%u, Data=%s\n", type, length, data);

  // Chiude il socket
  close(sock);

  return 0;
}
