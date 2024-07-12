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
#include "structs.h"

#define MAX_CLIENTS 32 // Definisce il numero massimo di client in attesa di connessione

Matrice mat;                                              // Variabile globale per la matrice
Utente *utenti_head = NULL;                               // Testa della lista degli utenti
pthread_mutex_t utenti_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex per la protezione della lista utenti
int tempo_attesa = 30;                                    // Tempo di attesa in secondi
int tempo_partita = 60;                                   // Tempo di partita in secondi



// Funzione per inviare la matrice al client
void handle_send_matrix(int client_socket, Matrice *mat)
{
  char response_type = MSG_MATRICE;                                // Tipo di messaggio
  unsigned int response_length = sizeof(Matrice);                  // Lunghezza del messaggio
  write(client_socket, &response_type, sizeof(response_type));     // Invia il tipo di messaggio
  write(client_socket, &response_length, sizeof(response_length)); // Invia la lunghezza del messaggio
  write(client_socket, mat, response_length);                      // Invia la matrice
}

// Funzione per inviare il tempo di attesa al client
void handle_send_tempo_attesa(int client_socket)
{
  char response_type = MSG_TEMPO_ATTESA;                           // Tipo di messaggio
  unsigned int response_length = sizeof(tempo_attesa);             // Lunghezza del messaggio
  write(client_socket, &response_type, sizeof(response_type));     // Invia il tipo di messaggio
  write(client_socket, &response_length, sizeof(response_length)); // Invia la lunghezza del messaggio
  write(client_socket, &tempo_attesa, response_length);            // Invia il tempo di attesa
}

// Funzione per inviare il tempo di partita al client
void handle_send_tempo_partita(int client_socket)
{
  char response_type = MSG_TEMPO_PARTITA;                          // Tipo di messaggio
  unsigned int response_length = sizeof(tempo_partita);            // Lunghezza del messaggio
  write(client_socket, &response_type, sizeof(response_type));     // Invia il tipo di messaggio
  write(client_socket, &response_length, sizeof(response_length)); // Invia la lunghezza del messaggio
  write(client_socket, &tempo_partita, response_length);           // Invia il tempo di partita
}

// Funzione per registrare un utente
int registra_utente(const char *nome)
{
  printf("Registrazione utente: %s\n", nome); // Stampa il nome dell'utente
  printf("Lunghezza nome: %ld\n", strlen(nome));
  pthread_mutex_lock(&utenti_mutex); // Blocca il mutex per la protezione della lista utenti

  if(strlen(nome) == 0 || strlen(nome) > 10){
    pthread_mutex_unlock(&utenti_mutex); // Sblocca il mutex
    return 0; // Nome utente vuoto
  }

  Utente *curr = utenti_head; // Puntatore al primo utente nella lista
  while (curr != NULL)
  { // Scorre la lista degli utenti
    if (strcmp(curr->nome, nome) == 0)
    {                                      // Controlla se l'utente esiste già
      pthread_mutex_unlock(&utenti_mutex); // Sblocca il mutex
      return 0;                            // Nome utente già esistente
    }
    curr = curr->next; // Passa al prossimo utente
  }

  Utente *nuovo_utente = (Utente *)malloc(sizeof(Utente));       // Alloca memoria per un nuovo utente
  strncpy(nuovo_utente->nome, nome, sizeof(nuovo_utente->nome)); // Copia il nome dell'utente
  nuovo_utente->punteggio = 0;                                   // Inizializza il punteggio a 0
  nuovo_utente->next = utenti_head;                              // Inserisce il nuovo utente all'inizio della lista
  utenti_head = nuovo_utente;                                    // Aggiorna la testa della lista

  pthread_mutex_unlock(&utenti_mutex); // Sblocca il mutex
  return 1;                            // Registrazione avvenuta con successo
}

// Funzione che verifica se la parola è valida nella matrice
int verifica_parola(const char *parola, Matrice *mat)
{
  // Questa è una versione semplificata che può essere espansa con la logica effettiva
  // Per ora assume che tutte le parole sono valide
  return 1;
}

// Funzione che calcola il punteggio di una parola
int calcola_punteggio(const char *parola)
{
  // Per ora assume un punto per ogni lettera
  return strlen(parola);
}

// Funzione per aggiornare il punteggio di un utente
void aggiorna_punteggio(const char *nome, int punteggio)
{
  pthread_mutex_lock(&utenti_mutex); // Blocca il mutex per la protezione della lista utenti

  Utente *curr = utenti_head; // Puntatore al primo utente nella lista
  while (curr != NULL)
  { // Scorre la lista degli utenti
    if (strcmp(curr->nome, nome) == 0)
    {                               // Trova l'utente con il nome specificato
      curr->punteggio += punteggio; // Aggiorna il punteggio dell'utente
      break;                        // Esce dal ciclo
    }
    curr = curr->next; // Passa al prossimo utente
  }

  pthread_mutex_unlock(&utenti_mutex); // Sblocca il mutex
}

// Funzione per gestire la ricezione di una parola dal client
void handle_parola(int client_socket, const char *nome, const char *parola)
{
  char response_type;           // Tipo di messaggio di risposta
  unsigned int response_length; // Lunghezza del messaggio di risposta
  char response_data[1024];     // Buffer per il messaggio di risposta

  if (verifica_parola(parola, &mat))
  {                                            // Verifica se la parola è valida
    int punteggio = calcola_punteggio(parola); // Calcola il punteggio della parola
    aggiorna_punteggio(nome, punteggio);       // Aggiorna il punteggio dell'utente

    response_type = MSG_PUNTI_PAROLA;                          // Imposta il tipo di messaggio a punti parola
    sprintf(response_data, "Punteggio parola: %d", punteggio); // Prepara il messaggio di risposta con il punteggio
  }
  else
  {
    response_type = MSG_ERR;                    // Imposta il tipo di messaggio a errore
    strcpy(response_data, "Parola non valida"); // Prepara il messaggio di errore
  }

  response_length = strlen(response_data); // Calcola la lunghezza del messaggio di risposta

  write(client_socket, &response_type, sizeof(response_type));     // Invia il tipo di messaggio
  write(client_socket, &response_length, sizeof(response_length)); // Invia la lunghezza del messaggio
  write(client_socket, response_data, response_length);            // Invia il messaggio di risposta
}

// Funzione per ricevere un messaggio dal client
void receive_message(int client_socket, char *type, unsigned int *length, char *data)
{
  read(client_socket, type, sizeof(char));           // Legge il tipo di messaggio
  read(client_socket, length, sizeof(unsigned int)); // Legge la lunghezza del messaggio
  read(client_socket, data, *length);                // Legge i dati del messaggio
  data[*length] = '\0';                              // Aggiunge il terminatore di stringa
}

// Funzione per gestire la comunicazione con il client
void *handle_client(void *client_socket)
{
  int sock = *(int *)client_socket; // Ottiene il file descriptor del socket del client
  free(client_socket);              // Libera la memoria allocata per il file descriptor

  char type;           // Tipo di messaggio
  unsigned int length; // Lunghezza del messaggio
  char data[1024];

  while (1)
  {                                   // Legge i dati dal client finché ci sono dati da leggere
    receive_message(sock, &type, &length, data);

    char response_type = MSG_OK;  // Tipo di messaggio di risposta predefinito
    unsigned int response_length; // Lunghezza del messaggio di risposta
    char response_data[1024];     // Buffer per il messaggio di risposta

    switch (type)
    {
      case MSG_REGISTRA_UTENTE:
        printf("Richiesta di registrazione utente: %s\n", data); // Stampa il nome utente
        if (registra_utente(data))
        {                                                               // Registra l'utente
          strcpy(response_data, "Registrazione avvenuta con successo"); // Prepara il messaggio di successo
        }
        else
        {
          // TODO controllare se il nome utente è più lungo di 10 caratteri 
          response_type = MSG_ERR;                            // Imposta il tipo di messaggio a errore
          strcpy(response_data, "Nome utente già esistente"); // Prepara il messaggio di errore
        }
        response_length = strlen(response_data); // Calcola la lunghezza del messaggio di risposta
        break;
      case MSG_MATRICE:
        handle_send_matrix(sock, &mat); // Invia la matrice al client
        continue;
      case MSG_TEMPO_ATTESA:
        handle_send_tempo_attesa(sock); // Invia il tempo di attesa al client
        continue;
      case MSG_TEMPO_PARTITA:
        handle_send_tempo_partita(sock); // Invia il tempo di partita al client
        continue;
      case MSG_PAROLA:
        // char *nome = data;                // Ottiene il nome utente dal messaggio
        // char *parola = nome + strlen(nome) + 1; // Ottiene la parola dal messaggio
        // handle_parola(sock, nome, parola);      // Gestisce la parola
        continue;
      default:
        response_type = MSG_ERR;                             // Imposta il tipo di messaggio a errore
        strcpy(response_data, "Messaggio non riconosciuto"); // Prepara il messaggio di errore
        response_length = strlen(response_data);             // Calcola la lunghezza del messaggio di risposta
        break;
    }

    write(sock, &response_type, sizeof(response_type));     // Invia il tipo di messaggio
    write(sock, &response_length, sizeof(response_length)); // Invia la lunghezza del messaggio
    write(sock, response_data, response_length);            // Invia il messaggio di risposta
  }

  close(sock); // Chiude il socket del client
  return NULL;
}

// Funzione principale
int main(int argc, char *argv[])
{
  int server_fd, new_socket;     // File descriptor per il server e il nuovo socket del client
  struct sockaddr_in address;    // Struttura per l'indirizzo del server
  int addrlen = sizeof(address); // Lunghezza dell'indirizzo

  if (argc >= 4)
  { // Controlla il numero di argomenti
    if (leggi_matrice_da_file(&mat, argv[3]) != 0)
    {                                                     // Legge la matrice dal file
      fprintf(stderr, "Errore nella lettura del file\n"); // Stampa un messaggio di errore se la lettura fallisce
      return 1;                                           // Termina il programma con errore
    }
    tempo_attesa = atoi(argv[4]);  // Ottiene il tempo di attesa dal secondo argomento
    tempo_partita = atoi(argv[5]); // Ottiene il tempo di partita dal terzo argomento
  }
  else
  {
    genera_matrice_casuale(&mat); // Genera una matrice casuale se non sono forniti argomenti sufficienti
  }

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {                          // Crea il socket del server
    perror("socket failed"); // Stampa un messaggio di errore se la creazione del socket fallisce
    exit(EXIT_FAILURE);      // Termina il programma con errore
  }

  address.sin_family = AF_INET;                 // Imposta la famiglia di indirizzi
  address.sin_port = htons(atoi(argv[2]));      // Imposta la porta del server
  address.sin_addr.s_addr = inet_addr(argv[1]); // Imposta l'indirizzo IP del server

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {                        // Associa il socket del server all'indirizzo e alla porta specificati
    perror("bind failed"); // Stampa un messaggio di errore se la bind fallisce
    close(server_fd);      // Chiude il socket del server
    exit(EXIT_FAILURE);    // Termina il programma con errore
  }

  if (listen(server_fd, MAX_CLIENTS) < 0)
  {                          // Imposta il socket del server in ascolto per le connessioni in ingresso
    perror("listen failed"); // Stampa un messaggio di errore se la listen fallisce
    close(server_fd);        // Chiude il socket del server
    exit(EXIT_FAILURE);      // Termina il programma con errore
  }

  printf("Server in ascolto su porta %d\n", atoi(argv[2])); // Stampa un messaggio indicando che il server è in ascolto sulla porta specificata

  while (1)
  { // Loop infinito per accettare e gestire le connessioni dei client
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {                          // Accetta una connessione in ingresso
      perror("accept failed"); // Stampa un messaggio di errore se l'accettazione della connessione fallisce
      continue;
    }

    printf("Connessione accettata\n"); // Stampa un messaggio indicando che la connessione è stata accettata

    pthread_t thread_id;                    // Crea un nuovo thread per gestire la comunicazione con il client
    int *client_sock = malloc(sizeof(int)); // Alloca memoria per il file descriptor del socket del client
    *client_sock = new_socket;              // Assegna il nuovo socket del client

    if (pthread_create(&thread_id, NULL, handle_client, (void *)client_sock) != 0)
    {                                  // Crea il thread e chiama la funzione handle_client
      perror("pthread_create failed"); // Stampa un messaggio di errore se la creazione del thread fallisce
      close(new_socket);               // Chiude il socket del client
      free(client_sock);               // Libera la memoria allocata per il file descriptor
    }
  }

  close(server_fd); // Chiude il socket del server (questo punto non verrà mai raggiunto a causa del loop infinito)
  return 0;         // Termina il programma con successo
}
