#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

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
  if (write(sock, &type, sizeof(type)) < 0)
  {
    perror("Errore nell'invio del tipo di messaggio");
    exit(EXIT_FAILURE);
  }
  // Invia la lunghezza del messaggio
  if (write(sock, &length, sizeof(length)) < 0)
  {
    perror("Errore nell'invio della lunghezza del messaggio");
    exit(EXIT_FAILURE);
  }
  // Invia i dati del messaggio
  if (write(sock, data, length) < 0)
  {
    perror("Errore nell'invio dei dati del messaggio");
    exit(EXIT_FAILURE);
  }
}

// Funzione per ricevere un messaggio
void receive_message(int sock, char *type, unsigned int *length, char *data)
{
  // Riceve il tipo di messaggio
  if (read(sock, type, sizeof(*type)) < 0)
  {
    perror("Errore nella ricezione del tipo di messaggio");
    exit(EXIT_FAILURE);
  }
  // Riceve la lunghezza del messaggio
  if (read(sock, length, sizeof(*length)) < 0)
  {
    perror("Errore nella ricezione della lunghezza del messaggio");
    exit(EXIT_FAILURE);
  }
  // Riceve i dati del messaggio
  if (read(sock, data, *length) < 0)
  {
    perror("Errore nella ricezione dei dati del messaggio");
    exit(EXIT_FAILURE);
  }
  data[*length] = '\0'; // Aggiunge il terminatore di stringa
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "Utilizzo: %s <nome_server> <porta>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int sock;
  struct sockaddr_in server_address;
  char *server_name = argv[1];
  int port = atoi(argv[2]); // Ottiene la porta del server dalla riga di comando

  // Crea il socket del client
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Errore nella creazione del socket");
    exit(EXIT_FAILURE);
  }

  struct hostent *host;
  if ((host = gethostbyname(server_name)) == NULL)
  {
    perror("Errore: nome del server non valido");
    exit(EXIT_FAILURE);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  memcpy(&server_address.sin_addr.s_addr, host->h_addr_list[0], sizeof(server_address.sin_addr.s_addr));

  printf("Tentativo di connessione al server %s sulla porta %d\n", server_name, port);  // Effettua la connessione al server
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

  // Gestione dei tipi di messaggio ricevuti
  switch (type)
  {
  case MSG_OK:
    printf("Registrazione avvenuta con successo\n");
    break;
  case MSG_ERR:
    printf("Errore: %s\n", data);
    break;
  // TODO Aggiungi altri casi per gestire altri tipi di messaggi come MSG_MATRICE, MSG_TEMPO_PARTITA, etc.
  default:
    printf("Tipo di messaggio sconosciuto ricevuto: %c\n", type);
    break;
  }

  // Chiude il socket
  close(sock);

  return 0;
}
