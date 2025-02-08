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
#include <getopt.h>

#include "matrice.h"
#include "structs.h"
#include "utilities.h"
#include <signal.h>

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

// Stato della partita: 0 = pausa (tempo di attesa), 1 = partita in corso.
volatile sig_atomic_t partitaInCorso = 0;

// Tempo residuo nell'intervallo corrente (in secondi)
volatile sig_atomic_t tempo_residuo = 0;

// Durate (in secondi)
int TEMPO_PARTITA = 60; // Valore da riga di comando (o default)
int TEMPO_PAUSA = 5;   // Sempre 60 secondi di pausa                                   // Tempo di partita (s)


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
  nuovo_utente->num_parole = 0;
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
  printf("Calcolo punteggio per parola");
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

void alarm_handler(int signum)
{
  // Decrementa il tempo residuo
  tempo_residuo--;

  // Se il tempo scade
  if (tempo_residuo <= 0)
  {
    if (partitaInCorso == 1)
    {
      // Se eravamo in partita, la partita è terminata: passiamo alla pausa
      partitaInCorso = 0;
      tempo_residuo = TEMPO_PAUSA;
      write(STDOUT_FILENO, "Partita terminata. Inizia la pausa.\n", 36);
    }
    else
    {
      // Se eravamo in pausa, la pausa è finita: inizia la partita
      partitaInCorso = 1;
      tempo_residuo = TEMPO_PARTITA;
      // Per la nuova partita, aggiorniamo la matrice (per esempio, prendiamo la successiva)
      get_next_matrice(&mat_attuale);
      write(STDOUT_FILENO, "Pausa terminata. La partita inizia.\n", 36);
    }
  }
  // Reimposta l'allarme per il prossimo secondo
  alarm(1);
}

// Funzione per gestire la ricezione di una parola (nome + parola)
void handle_parola(int client_socket, const char *nome, const char *parola)
{
  char response_type;
  char response_data[1024];
  memset(response_data, 0, sizeof(response_data));

  // Controllo duplicato: cerco l'utente nella lista degli utenti registrati
  int duplicato = 0;
  Utente *u = NULL;
  pthread_mutex_lock(&utenti_mutex);
  for (u = utenti_head; u != NULL; u = u->next)
  {
    if (strcmp(u->nome, nome) == 0)
    {
      // Ho trovato l'utente, ora controllo se la parola è già presente
      for (int i = 0; i < u->num_parole; i++)
      {
        if (strcmp(u->parole_usate[i], parola) == 0)
        {
          duplicato = 1;
          break;
        }
      }
      break;
    }
  }
  pthread_mutex_unlock(&utenti_mutex);

  if (duplicato)
  {
    // Se la parola è già stata proposta, invio 0 punti
    response_type = MSG_PUNTI_PAROLA;
    snprintf(response_data, sizeof(response_data), "Punteggio parola: %d", 0);
    send_message(client_socket, response_type, response_data);
    return;
  }

  // Verifica correttezza della parola (controlla dizionario, presenza nella matrice, lunghezza, ecc.)
  if (!verifica_parola(parola, &mat_attuale))
  {
    response_type = MSG_ERR;
    strcpy(response_data, "Parola non valida");
    send_message(client_socket, response_type, response_data);
    return;
  }

  // Se la parola è corretta e non è duplicata:
  int punteggio = calcola_punteggio(parola);
  // Aggiorna il punteggio dell'utente
  aggiorna_punteggio(nome, punteggio);

  // Aggiungo la parola alla lista dell'utente
  pthread_mutex_lock(&utenti_mutex);
  if (u != NULL && u->num_parole < MAX_PAROLE)
  {
    strncpy(u->parole_usate[u->num_parole], parola, MAX_LEN_PAROLA - 1);
    u->parole_usate[u->num_parole][MAX_LEN_PAROLA - 1] = '\0';
    u->num_parole++;
  }
  pthread_mutex_unlock(&utenti_mutex);

  response_type = MSG_PUNTI_PAROLA;
  snprintf(response_data, sizeof(response_data), "Punteggio parola: %d", punteggio);
  send_message(client_socket, response_type, response_data);
}

// Thread che gestisce un client specifico
void *handle_client(void *client_socket)
{
  int sock = *(int *)client_socket;
  free(client_socket);

  char type;
  int size;
  char data[1024];
  

  while (1)
  {
    // Leggiamo un messaggio dal client
    receive_message(sock, &type, &size, data);

    // Preparo la risposta
    char response_type = MSG_OK;
    char response_data[1024];
    memset(response_data, 0, sizeof(response_data));

    switch (type)
    {
    case MSG_REGISTRA_UTENTE:
      if (registra_utente(data))
      {
        char matrix_str[1024];
        matrice_to_string(&mat_attuale, matrix_str, sizeof(matrix_str));
        if (partitaInCorso == 1)
        {
          sprintf(response_data,
                  "Registrazione avvenuta con successo\n%s\nPartita in corso, tempo residuo: %d secondi",
                  matrix_str, tempo_residuo);
        }
        else
        {
          sprintf(response_data,
                  "Registrazione avvenuta con successo\nPartita pausa, tempo residuo: %d secondi",
                  tempo_residuo);
        }

        response_type = MSG_REGISTRA_UTENTE;
      }
      else
      {
        response_type = MSG_ERR;
        strcpy(response_data, "Nome utente già esistente o non valido");
      }
      break;

    case MSG_MATRICE:
      if (partitaInCorso == 1)
      {
      response_type = MSG_MATRICE;
      matrice_to_string(&mat_attuale, response_data, sizeof(response_data));
      break;
      }
      else
      {
        response_type = MSG_TEMPO_ATTESA;
        sprintf(response_data, "%d", tempo_residuo);
        break;
      }

    case MSG_TEMPO_ATTESA:
      response_type = MSG_TEMPO_ATTESA;
      sprintf(response_data, "%d", tempo_residuo);
      break;

    case MSG_TEMPO_PARTITA:
      response_type = MSG_TEMPO_PARTITA;
      sprintf(response_data, "%d", tempo_residuo);
      break;

    case MSG_PAROLA:
      if (partitaInCorso == 1){
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
      break;
      }
      else
      {
        response_type = MSG_ERR;
        strcpy(response_data, "Partita non ancora iniziata");
        break;
      }


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

int main(int argc, char *argv[])
{
  // Verifica degli argomenti minimi (nome_server e porta_server)
  if (argc < 3)
  {
    fprintf(stderr,
            "Uso: %s <nome_server> <porta_server> [--matrici data_filename] [--durata durata_in_minuti] [--seed rnd_seed] [--diz dizionario]\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }

  // Parametri obbligatori:
  char *nome_server = argv[1];
  char *porta_server = argv[2];

  // Parametri opzionali: inizializziamo con i valori di default
  char *matrici_filename = NULL;    // Se non specificato, genera matrici casuali
  int durata_minuti = 3;            // Durata di default = 3 minuti
  int rnd_seed = 0;                 // Se 0, non è stato specificato; altrimenti useremo il valore fornito
  char *dizionario_filename = NULL; // Dizionario opzionale

  // Utilizziamo getopt_long per analizzare gli argomenti opzionali
  int option_index = 0;
  int c;
  static struct option long_options[] = {
      {"matrici", required_argument, 0, 'm'},
      {"durata", required_argument, 0, 'd'},
      {"seed", required_argument, 0, 's'},
      {"diz", required_argument, 0, 'z'},
      {0, 0, 0, 0}};

  /* Gli argomenti obbligatori sono in argv[1] e argv[2],
     perciò getopt_long() inizierà ad analizzare argv a partire da argv[3]. */
  while ((c = getopt_long(argc, argv, "m:d:s:z:", long_options, &option_index)) != -1)
  {
    switch (c)
    {
    case 'm':
      matrici_filename = optarg;
      break;
    case 'd':
      durata_minuti = atoi(optarg);
      break;
    case 's':
      rnd_seed = atoi(optarg);
      break;
    case 'z':
      dizionario_filename = optarg;
      break;
    default:
      fprintf(stderr, "Opzione non riconosciuta\n");
      exit(EXIT_FAILURE);
    }
  }

  // Per il seed: se non è stato specificato, usiamo il tempo corrente
  if (rnd_seed == 0)
  {
    rnd_seed = (int)time(NULL);
  }

  // Stampa di debug dei parametri ottenuti
  printf("Nome server: %s\n", nome_server);
  printf("Porta server: %s\n", porta_server);
  if (matrici_filename)
    printf("File matrici: %s\n", matrici_filename);
  else
    printf("Generazione matrici casuale\n");
  printf("Durata partita: %d minuti\n", durata_minuti);
  printf("Seed: %d\n", rnd_seed);
  if (dizionario_filename)
    printf("File dizionario: %s\n", dizionario_filename);
  else
    printf("Nessun dizionario specificato\n");

  TEMPO_PARTITA = durata_minuti * 60;

  /* Se il file delle matrici è stato specificato, lo usiamo per caricare le matrici;
     altrimenti, generiamo una matrice casuale.
  */
  if (matrici_filename != NULL)
  {
    if (leggi_tutte_le_matrici(matrici_filename) != 0)
    {
      fprintf(stderr, "Errore nella lettura del file matrici: %s\n", matrici_filename);
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    // Genera matrice casuale
    genera_matrice_casuale(&mat_attuale);
    count_matrici = 1;
    array_matrici[0] = mat_attuale;
  }

  /* Imposta lo stato iniziale del gioco.
     Per esempio, il server parte in pausa (tempo di attesa).
  */
  partitaInCorso = 0;
  tempo_residuo = TEMPO_PAUSA;

  /* Installa il gestore del segnale SIGALRM usando signal().
     Il gestore, alarm_handler(), gestirà il countdown e il cambio di stato.
  */
  if (signal(SIGALRM, alarm_handler) == SIG_ERR)
  {
    perror("Errore nell'installazione del gestore SIGALRM");
    exit(EXIT_FAILURE);
  }

  /* Avvia l'allarme: il primo SIGALRM scatta tra 1 secondo.
     Il gestore alarm_handler() verrà richiamato ogni secondo.
  */
  alarm(1);

  /* Imposta la matrice iniziale per la partita (se vuoi che la prima partita usi la prima matrice)
     Si utilizza get_next_matrice() per ottenere la matrice successiva (in modo circolare)
  */
  get_next_matrice(&mat_attuale);

  // Eventuale impostazione del seed per la generazione pseudocasuale
  srand(rnd_seed);

  // Se hai un file dizionario, potresti volerlo aprire/inizializzare qui (non mostrato in questo esempio)

  int server_fd;
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(porta_server));
  address.sin_addr.s_addr = inet_addr(nome_server);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, MAX_CLIENTS) < 0)
  {
    perror("listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server in ascolto su %s:%s ...\n", nome_server, porta_server);

  // Qui, eventualmente, potresti creare un thread per il broadcast (se implementato)
  // Ad esempio:
  // pthread_t b_thread;
  // if (pthread_create(&b_thread, NULL, broadcaster_thread, NULL) != 0) { ... }

  // Loop principale per accettare i client
  while (1)
  {
    int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0)
    {
      perror("accept failed");
      continue;
    }
    printf("Connessione accettata da un client.\n");

    // Qui potresti aggiungere il nuovo socket alla lista dei client per il broadcast

    pthread_t thread_id;
    int *client_sock = malloc(sizeof(int));
    *client_sock = new_socket;
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