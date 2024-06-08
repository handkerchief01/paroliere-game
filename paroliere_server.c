#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <pthread.h>
#include "matrice.h"

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

#define MAX_CLIENTS 10 // Definisce il numero massimo di client in attesa di connessione

void *handle_client(void *client_socket) // Funzione per gestire la comunicazione con il client
{

  int sock = *(int *)client_socket; // Ottiene il file descriptor del socket del client

  free(client_socket); // Libera la memoria allocata per il file descriptor

  char buffer[1024]; // Buffer per la lettura dei dati dal client
  int bytes_read;

  while ((bytes_read = read(sock, buffer, sizeof(buffer) - 1)) > 0) // Legge i dati dal client finché ci sono dati da leggere
  {

    buffer[bytes_read] = '\0'; // Aggiunge il terminatore di stringa alla fine del buffer

    printf("Ricevuto: %s\n", buffer); // Stampa il messaggio ricevuto dal client

    char response_type = MSG_OK; // Elabora il messaggio ricevuto e crea la risposta
    unsigned int response_length = strlen("MSG_OK");
    char response_data[] = "MSG_OK";

    write(sock, &response_type, sizeof(response_type)); // Invia il tipo di messaggio

    write(sock, &response_length, sizeof(response_length)); // Invia la lunghezza del messaggio

    write(sock, response_data, response_length); // Invia i dati del messaggio
  }

  close(sock); // Chiude il socket del client
  return NULL;
}

int main(int argc, char *argv[])
{
  int server_fd, new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  Matrice mat; // Dichiarazione di una variabile di tipo Matrice
  int port = atoi(argv[1]); // Ottiene la porta del server dalla riga di comando

  printf("numero argomenti: %d\n", argc);

  if (argc == 3)
  {
    // Leggere la matrice dal file
    if (leggi_matrice_da_file(&mat, argv[1]) != 0)
    {
      fprintf(stderr, "Errore nella lettura del file\n");
      return 1;
    }
  }
  else
  {
    // Generare una matrice casuale
    genera_matrice_casuale(&mat);
  }

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // Crea il socket del server
  {

    perror("socket failed"); // Se la creazione del socket fallisce, stampa un messaggio di errore e termina il programma
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET; // Imposta la famiglia di indirizzi

  address.sin_port = htons(port); // Imposta la porta del server
  
  address.sin_addr.s_addr = INADDR_ANY; // Imposta l'indirizzo IP del server (accetta connessioni da qualsiasi indirizzo)

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) // Associa il socket del server all'indirizzo e alla porta specificati
  {

    perror("bind failed"); // Se la bind fallisce, stampa un messaggio di errore, chiude il socket del server e termina il programma
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, MAX_CLIENTS) < 0) // Imposta il socket del server in ascolto per le connessioni in ingresso
  {

    perror("listen failed"); // Se la listen fallisce, stampa un messaggio di errore, chiude il socket del server e termina il programma
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server in ascolto su porta %d\n", port); // Stampa un messaggio indicando che il server è in ascolto sulla porta specificata

  while (1) // Loop infinito per accettare e gestire le connessioni dei client
  {

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) // Accetta una connessione in ingresso
    {

      perror("accept failed"); // Se l'accettazione della connessione fallisce, stampa un messaggio di errore e continua
      continue;
    }

    printf("Connessione accettata\n"); // Stampa un messaggio indicando che la connessione è stata accettata

    pthread_t thread_id; // Crea un nuovo thread per gestire la comunicazione con il client

    int *client_sock = malloc(sizeof(int)); // Alloca memoria per il file descriptor del socket del client
    *client_sock = new_socket;

    if (pthread_create(&thread_id, NULL, handle_client, (void *)client_sock) != 0) // Crea il thread e chiama la funzione handle_client
    {
      perror("pthread_create failed"); // Se la creazione del thread fallisce, stampa un messaggio di errore, chiude il socket del client e libera la memoria
      close(new_socket);
      free(client_sock);
    }
  }

  close(server_fd); // Chiude il socket del server (questo punto non verrà mai raggiunto a causa del loop infinito)
  return 0;
}
