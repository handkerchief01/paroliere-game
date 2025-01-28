#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "matrice.h"
#include "structs.h"
#include "utilities.h"

// Numero massimo di client in coda di connessione
#define MAX_CLIENTS 32
#define MAX_MATRICI 100 // numero massimo di righe/matrici nel file

// Variabili globali
Matrice mat_attuale;                                      // Matrice di gioco 4x4
Utente *utenti_head = NULL;                               // Lista utenti collegati
pthread_mutex_t utenti_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex per proteggere la lista

Matrice array_matrici[MAX_MATRICI];                       // tutte le matrici lette
int count_matrici = 0;                                    // quante righe/matrici abbiamo letto
int current_index = 0;                                    // indice della prossima matrice da usare

int tempo_attesa = 30;                                    // Tempo di attesa (s)
int tempo_partita = 60;                                   // Tempo di partita (s)

// Funzione per registrare un utente nella lista
int registra_utente(const char *nome)
{
  printf("Registrazione utente: %s\n", nome);

  pthread_mutex_lock(&utenti_mutex);

  // Verifichiamo la lunghezza minima/massima
  if (strlen(nome) == 0 || strlen(nome) > 10)
  {
    pthread_mutex_unlock(&utenti_mutex);
    return 0; // Nome non valido
  }

  // Verifichiamo che il nome contenga solo caratteri ammessi (alfabeto italiano e numeri)
  for (int i = 0; i < strlen(nome); i++)
  {
    if (!is_italian_alnum(nome[i]))
    {
      pthread_mutex_unlock(&utenti_mutex);
      return 0; // Carattere non ammesso
    }
  }

  // Controllo se il nome utente è già presente
  Utente *curr = utenti_head;
  while (curr != NULL)
  {
    if (strcmp(curr->nome, nome) == 0)
    {
      pthread_mutex_unlock(&utenti_mutex);
      return 0; // Nome già esistente
    }
    curr = curr->next;
  }

  // Creazione nuovo utente
  Utente *nuovo_utente = (Utente *)malloc(sizeof(Utente));
  strncpy(nuovo_utente->nome, nome, sizeof(nuovo_utente->nome));
  nuovo_utente->nome[sizeof(nuovo_utente->nome) - 1] = '\0';
  nuovo_utente->punteggio = 0;
  nuovo_utente->next = utenti_head;
  utenti_head = nuovo_utente;

  pthread_mutex_unlock(&utenti_mutex);
  return 1;
}

// Funzione per verificare se la parola è valida nella matrice
int verifica_parola(const char *parola, const Matrice *mat)
{
  // Per ora ritorna 1 (tutte valide)
  return 1;
}

// Calcola il punteggio di una parola (stub semplificato: lunghezza parola)
int calcola_punteggio(const char *parola)
{
  return (int)strlen(parola);
}

// Aggiorna il punteggio di un utente
void aggiorna_punteggio(const char *nome, int punteggio)
{
  pthread_mutex_lock(&utenti_mutex);
  Utente *curr = utenti_head;
  while (curr != NULL)
  {
    if (strcmp(curr->nome, nome) == 0)
    {
      curr->punteggio += punteggio;
      break;
    }
    curr = curr->next;
  }
  pthread_mutex_unlock(&utenti_mutex);
}

// Funzione per gestire la ricezione di una parola (nome + parola)
void handle_parola(int client_socket, const char *nome, const char *parola)
{
  char response_type;
  char response_data[1024];
  memset(response_data, 0, sizeof(response_data));

  if (verifica_parola(parola, &mat_attuale))
  {
    int punteggio = calcola_punteggio(parola);
    aggiorna_punteggio(nome, punteggio);

    response_type = MSG_PUNTI_PAROLA;
    sprintf(response_data, "Punteggio parola: %d", punteggio);
  }
  else
  {
    response_type = MSG_ERR;
    strcpy(response_data, "Parola non valida");
  }

  send_message(client_socket, response_type, response_data);
}

// Thread che gestisce un client specifico
void *handle_client(void *client_socket)
{
  int sock = *(int *)client_socket;
  free(client_socket);

  char type;           // Tipo di messaggio
  unsigned int length; // Lunghezza dei dati
  char data[1024];     // Buffer di ricezione

  while (1)
  {
    // Leggiamo un messaggio dal client
    receive_message(sock, &type, &length, data);

    // Preparo la risposta
    char response_type = MSG_OK;
    char response_data[1024];
    memset(response_data, 0, sizeof(response_data));

    switch (type)
    {
    case MSG_REGISTRA_UTENTE:
      if (registra_utente(data))
      {
        strcpy(response_data, "Registrazione avvenuta con successo");
      }
      else
      {
        response_type = MSG_ERR;
        strcpy(response_data, "Nome utente già esistente o non valido");
      }
      break;

    case MSG_MATRICE:
      // Convertiamo la matrice in formato testuale
      response_type = MSG_MATRICE;
      matrice_to_string(&mat_attuale, response_data, sizeof(response_data));
      break;

    case MSG_TEMPO_ATTESA:
      response_type = MSG_TEMPO_ATTESA;
      sprintf(response_data, "%d", tempo_attesa);
      break;

    case MSG_TEMPO_PARTITA:
      response_type = MSG_TEMPO_PARTITA;
      sprintf(response_data, "%d", tempo_partita);
      break;

    case MSG_PAROLA:
      // Esempio: ipotizziamo `data` contenga "NOME\0PAROLA"
      // Occorre separare il nome dalla parola.
      // Oppure puoi decidere un protocollo diverso.
      // Per ora ipotizziamo di doverlo fare manualmente:
      {
        char nome[50];
        char parola[50];
        memset(nome, 0, sizeof(nome));
        memset(parola, 0, sizeof(parola));

        // Copio la prima stringa (nome)
        strcpy(nome, data);
        // La parola inizia subito dopo il '\0' del nome
        const char *p = data + strlen(nome) + 1;
        strcpy(parola, p);

        // Gestiamo la parola
        handle_parola(sock, nome, parola);
      }
      // Gestito direttamente, saltiamo il send_message sotto
      continue;
      break;

    default:
      response_type = MSG_ERR;
      strcpy(response_data, "Messaggio non riconosciuto");
      break;
    }

    // Invia la risposta
    send_message(sock, response_type, response_data);
  }

  close(sock);
  return NULL;
}

int leggi_tutte_le_matrici(const char *filename)
{
  FILE *f = fopen(filename, "r");
  if (!f)
  {
    perror("Errore apertura file matrici");
    return -1;
  }
  count_matrici = 0;

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), f))
  {
    // Stampiamo la riga grezza letta (debug)
    printf("Riga letta: '%s'\n", buffer);

    // Se la riga è vuota, skip
    if (buffer[0] == '\n' || buffer[0] == '\0')
    {
      printf(" -> Riga vuota, salto.\n");
      continue;
    }

    // Parse 16 token
    char *token = strtok(buffer, " \t\r\n");
    int tokenCount = 0;
    while (token && tokenCount < 16)
    {
      printf("   Token #%d: '%s'\n", tokenCount + 1, token); // debug
      strncpy(array_matrici[count_matrici].matrice[tokenCount / 4][tokenCount % 4],
              token, 3);
      array_matrici[count_matrici].matrice[tokenCount / 4][tokenCount % 4][3] = '\0';
      tokenCount++;
      token = strtok(NULL, " \t\r\n");
    }
    printf(" => Trovati %d token in questa riga.\n", tokenCount);

    // Se la riga ha 16 token validi, contiamo questa matrice
    if (tokenCount == 16)
    {
      count_matrici++;
      printf(" -> OK, matrice %d caricata.\n", count_matrici);
      if (count_matrici >= MAX_MATRICI)
      {
        printf("Raggiunto il limite di %d matrici.\n", MAX_MATRICI);
        break;
      }
    }
    else
    {
      printf(" -> Riga con tokenCount != 16, ignorata.\n");
    }
  }

  fclose(f);

  printf("Totale matrici caricate: %d\n", count_matrici);
  return 0;
}

int get_next_matrice(Matrice *dest)
{
  if (count_matrici == 0)
  {
    fprintf(stderr, "Nessuna matrice caricata!\n");
    return -1;
  }

  // Copia la matrice dall'array in dest
  *dest = array_matrici[current_index];

  // Avanziamo l'indice in modo circolare (se vuoi riusare in loop)
  // o lo incrementi fino a fermarti all'ultima
  current_index = (current_index + 1) % count_matrici;
  return 0;
}

// Funzione principale del server
int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, "Uso: %s <IP> <PORTA> [file_matrice] [tempo_attesa] [tempo_partita]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Se c'è un file di matrici, lo leggiamo tutto
  if (argc >= 4)
  {
    if (leggi_tutte_le_matrici(argv[3]) != 0)
    {
      fprintf(stderr, "Errore nella lettura del file %s\n", argv[3]);
      return 1;
    }
  }
  else
  {
    genera_matrice_casuale(&mat_attuale);
    count_matrici = 1;
  }

  // Eventuali parametri di tempo
  if (argc >= 5)
    tempo_attesa = atoi(argv[4]);
  if (argc >= 6)
    tempo_partita = atoi(argv[5]);

  // Impostiamo la matrice iniziale (se vuoi che la prima partita inizi con la prima riga)
  // Oppure la prendi ogni volta che parte davvero la partita, dipende da te
  get_next_matrice(&mat_attuale);

  // Nel resto del server 'mat_attuale' è la matrice "corrente". Oppure
  // potresti salvare 'mat_attuale' in una variabile globale se serve.
  // ...
  // Oppure nel tuo `handle_client` o dove fai partire una nuova partita,
  // chiami get_next_matrice(&mat_attuale).

  // Creazione socket
  int server_fd;
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Configuriamo indirizzo e porta
  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(argv[2]));
  address.sin_addr.s_addr = inet_addr(argv[1]);

  // Bind
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // Listen
  if (listen(server_fd, MAX_CLIENTS) < 0)
  {
    perror("listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server in ascolto su %s:%s ...\n", argv[1], argv[2]);

  // Loop per accettare i client
  while (1)
  {
    int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0)
    {
      perror("accept failed");
      continue;
    }

    printf("Connessione accettata da un client.\n");

    // Crea un thread per gestire il nuovo client
    pthread_t thread_id;
    int *client_sock = malloc(sizeof(int));
    *client_sock = new_socket;

    // Esempio: se vuoi che ogni client parta con la "prossima" matrice:
    // get_next_matrice(&mat_attuale);
    // ... e passi mat_attuale al thread in qualche modo
    // (per ora potresti passare un puntatore globale, dipende dal design).
    // Oppure la matrice si cambia solo a inizio partita, etc.

    if (pthread_create(&thread_id, NULL, handle_client, (void *)client_sock) != 0)
    {
      perror("pthread_create failed");
      close(new_socket);
      free(client_sock);
    }
  }

  close(server_fd);
  return 0;
}