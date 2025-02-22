#define _XOPEN_SOURCE 700 // perché sigaction fa parte di POSIX.1-2008
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
#include <signal.h>
#include <sys/time.h>

#include "matrice.h"
#include "structs.h"
#include "utilities.h"
#include "trie.h"
#include "server_utilities.h"

// Numero massimo di client in coda di connessione
#define MAX_CLIENTS 32
#define MAX_MATRICI 100 // numero massimo di righe/matrici nel file
#define MAX_BACHECA 8

// Variabili globali
ClientNode *clientList = NULL;
pthread_mutex_t clientListMutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t shutdownRequested = 0;

Matrice mat_attuale;                                      // Matrice di gioco 4x4
volatile sig_atomic_t updateMatrixFlag = 0;

Utente *utenti_head = NULL;                               // Lista utenti collegati
pthread_mutex_t utenti_mutex = PTHREAD_MUTEX_INITIALIZER;

Matrice array_matrici[MAX_MATRICI];                       // tutte le matrici lette
int count_matrici = 0;                                    // quante righe/matrici abbiamo letto
int current_index = 0;                                    // indice della prossima matrice da usare
TrieNode *dictionaryRoot = NULL;

// Stato della partita: 0 = pausa (tempo di attesa), 1 = partita in corso.
volatile sig_atomic_t partitaInCorso = 0;
// Tempo residuo nell'intervallo corrente (in secondi)
volatile sig_atomic_t tempo_residuo = 0;
// Durate (in secondi)
int TEMPO_PARTITA = 60; // Valore da riga di comando (o default)
int TEMPO_PAUSA = 5;   // Sempre 60 secondi di pausa

// Variabile globale per memorizzare i minuti dopo cui disconnettere
static int DISCONNECT_AFTER = 0;

// Struttura per la bacheca dei messaggi
static BachecaMsg bacheca[MAX_BACHECA];
static int bacheca_count = 0; // quanti messaggi attualmente in bacheca (<= 8)
static int bacheca_index = 0; // prossima posizione di scrittura (indice circolare)
pthread_mutex_t bacheca_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t partitaTerminataFlag = 0; // Indica che la partita è appena terminata, mi serve per lo scorer

void add_client(int sock)
{
  ClientNode *node = malloc(sizeof(ClientNode));
  if (!node)
  {
    perror("Errore allocazione client node");
    return;
  }
  node->sock = sock;
  pthread_mutex_lock(&clientListMutex);
  node->next = clientList;
  clientList = node;
  pthread_mutex_unlock(&clientListMutex);
}

void remove_client(int sock)
{
  pthread_mutex_lock(&clientListMutex);
  ClientNode *curr = clientList, *prev = NULL;
  while (curr != NULL)
  {
    if (curr->sock == sock)
    {
      if (prev == NULL)
        clientList = curr->next;
      else
        prev->next = curr->next;
      free(curr);
      break;
    }
    prev = curr;
    curr = curr->next;
  }
  pthread_mutex_unlock(&clientListMutex);
}

void sigint_handler(int signum)
{
  const char *file = "log.txt";
  remove(file);
  shutdownRequested = 1;
}

int post_bacheca(const char *user, const char *text)
{
  // Controlla lunghezza text
  if (strlen(text) > MAX_MSG_LEN)
  {
    return 0; // errore: troppo lungo
  }

  pthread_mutex_lock(&bacheca_mutex);

  // Scriviamo in bacheca[bacheca_index]
  strncpy(bacheca[bacheca_index].user, user, sizeof(bacheca[bacheca_index].user) - 1);
  bacheca[bacheca_index].user[sizeof(bacheca[bacheca_index].user) - 1] = '\0';

  strncpy(bacheca[bacheca_index].text, text, MAX_MSG_LEN);
  bacheca[bacheca_index].text[MAX_MSG_LEN] = '\0';

  // Avanziamo indice in modo circolare
  bacheca_index = (bacheca_index + 1) % MAX_BACHECA;

  // Se non siamo ancora a 8 messaggi, incrementa count
  if (bacheca_count < MAX_BACHECA)
  {
    bacheca_count++;
  }

  pthread_mutex_unlock(&bacheca_mutex);

  return 1; // OK
}

/**
 * Costruisce un CSV con i messaggi in ordine dal più vecchio al più nuovo.
 * Nel buffer out (size out_size) inseriamo qualcosa tipo:
 *   "pippo|Ciao, pluto|Benvenuto, ..."
 * Ritorna quanti byte abbiamo scritto, o -1 se errore.
 */
int show_bacheca(char *out, size_t out_size)
{
  pthread_mutex_lock(&bacheca_mutex);

  // Scrivi l'intestazione
  const char *header = "\"Utente\",\"Messaggio\"\n";
  size_t used = 0;
  size_t hdr_len = strlen(header);

  if (hdr_len >= out_size)
  {
    pthread_mutex_unlock(&bacheca_mutex);
    return -1; // buffer troppo piccolo
  }

  strcpy(out, header);
  used = hdr_len;

  // Calcola l'indice del messaggio "più vecchio"
  int oldest = (bacheca_index - bacheca_count + MAX_BACHECA) % MAX_BACHECA;

  // Aggiungi i record
  for (int i = 0; i < bacheca_count; i++)
  {
    int idx = (oldest + i) % MAX_BACHECA;

    int written = snprintf(
        out + used,
        out_size - used,
        "\"%s\",\"%s\"\n",
        bacheca[idx].user,  // utente
        bacheca[idx].text); // messaggio

    if (written < 0 || (size_t)written >= (out_size - used))
    {
      pthread_mutex_unlock(&bacheca_mutex);
      return -1; // overflow
    }
    used += written;
  }

  pthread_mutex_unlock(&bacheca_mutex);
  return (int)used;
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
      partitaTerminataFlag = 1;
      tempo_residuo = TEMPO_PAUSA;
      write(STDOUT_FILENO, "Partita terminata. Inizia la pausa.\n", 36);
    }
    else
    {
      // Se eravamo in pausa, la pausa è finita: inizia la partita
      partitaInCorso = 1;
      tempo_residuo = TEMPO_PARTITA;
      // Per la nuova partita, aggiorniamo la matrice (per esempio, prendiamo la successiva)
      updateMatrixFlag = 1;
      write(STDOUT_FILENO, "Pausa terminata. La partita inizia.\n", 36);
    }
  }
  // Reimposta l'allarme per il prossimo secondo
  alarm(1);
}

void *controller_thread(void *arg)
{
  while (!shutdownRequested)
  {
    // Aspetta che partitaTerminataFlag diventi 1
    // (puoi usare un pthread_cond, oppure un semplice "while" con piccolo sleep)
    while (!partitaTerminataFlag && !shutdownRequested)
    {
      sleep(1);
    }
    if (shutdownRequested)
      break;

    // Se siamo qui, la partita è finita
    partitaTerminataFlag = 0;

    // Calcola classifica
    char scoreboard[1024];
    build_classifica_csv(scoreboard, sizeof(scoreboard));

    // Invia a tutti
    pthread_mutex_lock(&clientListMutex);
    ClientNode *c = clientList;
    while (c != NULL)
    {
      invia_messaggio(c->sock, MSG_PUNTI_FINALI, scoreboard);
      c = c->next;
    }
    pthread_mutex_unlock(&clientListMutex);
  }
  return NULL;
}

// Thread che gestisce un client specifico
void *handle_client(void *client_socket)
{
  int sock = *(int *)client_socket;
  free(client_socket);

  if (DISCONNECT_AFTER > 0)
  {
    struct timeval tv;
    tv.tv_sec = DISCONNECT_AFTER * 60; // minuti -> secondi
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
  }
  
  char type;
  int size;
  char data[1024];

  while (1)
  {
    // Leggiamo un messaggio dal client
    int n = ricevi_messaggio(sock, &type, &size, data);
    if (n == 0)
    {
      // EOF => client ha chiuso
      printf("Client disconnesso (sock=%d)\n", sock);
      close(sock);
      remove_client(sock);
      return NULL;
    }
    else if (n < 0)
    {
      // Errore => controlliamo se è EAGAIN
      if (errno == EAGAIN)
      {
        printf("Client inattivo da troppo tempo. Espulsione (sock=%d).\n", sock);
      }
      else
      {
        perror("ricevi_messaggio() fallita");
      }
      close(sock);
      remove_client(sock);
      return NULL;
    }

    // Preparo la risposta
    char response_type = MSG_OK;
    char response_data[1024];
    memset(response_data, 0, sizeof(response_data));

    switch (type)
    {
    case MSG_REGISTRA_UTENTE:
      if (registra_utente(data))
      {
        log_event("Registrazione utente: %s", data);
        char matrix_str[512];
        matrice_to_string(&mat_attuale, matrix_str, sizeof(matrix_str));
        if (partitaInCorso == 1)
        {
          snprintf(response_data, sizeof(response_data), "Registrazione avvenuta con successo\n%s\nPartita in corso, tempo residuo: %d secondi",
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

    case MSG_CANCELLA_UTENTE:
      if (cancella_utente(data))
      {
        log_event("Cancellazione utente: %s", data);
        invia_messaggio(sock, MSG_CANCELLA_UTENTE, "Cancellazione avvenuta con successo");
      }
      else
      {
        invia_messaggio(sock, MSG_ERR, "Utente non trovato o cancellazione fallita");
      }
      break;

    case MSG_LOGIN_UTENTE:
      if (login_utente(data))
      {
        log_event("Login utente: %s", data);
        invia_messaggio(sock, MSG_LOGIN_UTENTE, "Login avvenuto con successo");
      }
      else
      {
        invia_messaggio(sock, MSG_ERR, "Utente non registrato");
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
      if (partitaInCorso == 1)
      {
        char nome[20], parola[32];
        // Usa "|" come delimitatore
        char *tokenNome = strtok(data, "|");
        char *tokenParola = strtok(NULL, "|");
        if (tokenNome != NULL && tokenParola != NULL)
        {
          strcpy(nome, tokenNome);
          strcpy(parola, tokenParola);
          printf("Nome: %s, Parola: %s\n", nome, parola);
          handle_parola(sock, nome, parola);
        }
        else
        {
          // Se il formato non è corretto
          response_type = MSG_ERR;
          strcpy(response_data, "Formato del messaggio non valido");
          invia_messaggio(sock, response_type, response_data);
        }
        continue;
      }
      else
      {
        response_type = MSG_ERR;
        strcpy(response_data, "Partita non ancora iniziata");
      }
      break;

    case MSG_POST_BACHECA:
    {
      char *tokenUtente = strtok(data, "|");
      char *tokenTesto = strtok(NULL, "");
      // se tokenTesto è NULL => formattazione errata
      if (!tokenUtente || !tokenTesto)
      {
        invia_messaggio(sock, MSG_ERR, "Formato messaggio bacheca non valido");
        break;
      }
      // se vuoi controllare che l'utente esista già:
      // if (!login_utente(tokenUtente)) { ... MSG_ERR ... }

      // Salva in bacheca
      if (!post_bacheca(tokenUtente, tokenTesto))
      {
        // se è fallito perché > MAX_MSG_LEN
        invia_messaggio(sock, MSG_ERR, "Messaggio troppo lungo");
      }
      else
      {
        invia_messaggio(sock, MSG_OK, "Messaggio registrato in bacheca");
      } 
    break;
    }

    case MSG_SHOW_BACHECA:
    {
      char buffer[2048];
      int n = show_bacheca(buffer, sizeof(buffer));

      if (n < 0)
      {
        invia_messaggio(sock, MSG_ERR, "Errore nella formattazione bacheca");
      }
      else
      {
        invia_messaggio(sock, MSG_SHOW_BACHECA, buffer);
      }
      break;
    }

    default:
      response_type = MSG_ERR;
      strcpy(response_data, "Messaggio non riconosciuto");
      break;
    }

    // Invia la risposta
    invia_messaggio(sock, response_type, response_data);
  }

  close(sock);
  remove_client(sock);
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
  signal(SIGPIPE, SIG_IGN);

  struct sigaction sa_int;
  memset(&sa_int, 0, sizeof(sa_int));
  sa_int.sa_handler = sigint_handler;
  if (sigaction(SIGINT, &sa_int, NULL) == -1)
  {
    perror("sigaction(SIGINT) error");
    exit(EXIT_FAILURE);
  }
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
      {"disconnetti-dopo", required_argument, 0, 'x'},
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
    case 'x':
      DISCONNECT_AFTER = atoi(optarg);
      printf("Timeout di inattivita' impostato a %d minuti\n", DISCONNECT_AFTER);
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

  if (dizionario_filename != NULL)
  {
    dictionaryRoot = loadDictionary(dizionario_filename);
    if (!dictionaryRoot)
    {
      fprintf(stderr, "Errore nel caricamento del dizionario.\n");
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    dictionaryRoot = loadDictionary("dizionario.txt");
    if (!dictionaryRoot)
    {
      fprintf(stderr, "Errore nel caricamento del dizionario.\n");
      exit(EXIT_FAILURE);
    }
  }

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

  // Gestione tempo di gioco con alarm()
  struct sigaction sa_alrm;
  memset(&sa_alrm, 0, sizeof(sa_alrm));
  sa_alrm.sa_handler = alarm_handler;
  if (sigaction(SIGALRM, &sa_alrm, NULL) == -1)
  {
    perror("sigaction(SIGALRM) error");
    exit(EXIT_FAILURE);
  }
  alarm(1);

  /* Imposta la matrice iniziale per la partita
  */
  get_next_matrice(&mat_attuale);

  // Eventuale impostazione del seed per la generazione pseudocasuale
  srand(rnd_seed);

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

  pthread_t ctrl_tid;
  pthread_create(&ctrl_tid, NULL, controller_thread, NULL);
  // Loop principale per accettare i client
  while (!shutdownRequested)
  {
    int new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0)
    {
      if (errno == EINTR)
      {
        // La chiamata è stata interrotta da un segnale (ad esempio SIGALRM o SIGINT)
        continue;
      }
      perror("accept failed");
      continue;
    }
    printf("Connessione accettata da un client.\n");

    add_client(new_socket);

    pthread_t thread_id;
    int *client_sock = malloc(sizeof(int));
    *client_sock = new_socket;
    if (pthread_create(&thread_id, NULL, handle_client, (void *)client_sock) != 0)
    {
      perror("pthread_create failed");
      close(new_socket);
      free(client_sock);
      remove_client(new_socket);
    }

    if (updateMatrixFlag)
    {
      get_next_matrice(&mat_attuale);
      updateMatrixFlag = 0;
    }
  }

  pthread_mutex_lock(&clientListMutex);
  ClientNode *curr = clientList;
  while (curr != NULL)
  {
    invia_messaggio(curr->sock, MSG_SERVER_SHUTDOWN, "Server in chiusura");
    curr = curr->next;
  }
  pthread_mutex_unlock(&clientListMutex);

  sleep(1); // il tempo che il client riceva il messaggio di chiusura

  // Chiudi il socket di ascolto
  close(server_fd);
  printf("Server terminato.\n");
  return 0;
}