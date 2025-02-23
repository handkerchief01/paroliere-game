#define _XOPEN_SOURCE 600 // Definisce la macro per conformità POSIX, senza di questo non riesce ad andare addrinfo
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "macros.h"
#include <netdb.h>
#include <pthread.h>
#include <signal.h>

#include "structs.h"
#include "utilities.h"

char my_name[20] = "";
int is_registered = 0;

void *receiver_thread(void *arg)
{
  int sock = *(int *)arg;
  char type;
  int size;
  char data[1024];

  while (1)
  {
    int n = ricevi_messaggio(sock, &type, &size, data);

    if (n <= 0)
    {
      printf("Connessione chiusa per disconnessione o errore di lettura.\n");
      exit(0);
      break;
    }

    switch (type)
    {
    case MSG_REGISTRA_UTENTE:
      printf("Risposta dal server:\n%s\n", data);
      is_registered = 1; // Imposta lo stato come registrato
      my_name[sizeof(my_name) - 1] = '\0';
      break;

    case MSG_LOGIN_UTENTE:
      printf("Risposta dal server:\n%s\n", data);
      is_registered = 1; // Imposta lo stato come registrato
      my_name[sizeof(my_name) - 1] = '\0';
      break;

    case MSG_CANCELLA_UTENTE:
      printf("Risposta dal server:\n%s\n", data);
      is_registered = 0;
      my_name[0] = '\0';
      break;

    case MSG_MATRICE:
      printf("Matrice ricevuta:\n%s\n", data);
      break;


    case MSG_TEMPO_PARTITA:
      // Se il server è in pausa e invia il tempo di attesa residuo
      printf("Tempo residuo della partita: %s secondi\n", data);
      break;

    case MSG_TEMPO_ATTESA:
      // Se il server è in pausa e invia il tempo di attesa residuo
      printf("Partita in pausa. Tempo residuo di attesa: %s secondi\n", data);
      break;

    case MSG_PUNTI_PAROLA:
      printf("%s\n", data);
      break;

    case MSG_SHOW_BACHECA:
      printf("BACHECA:\n");
      printf("%s\n", data); // ad esempio: stampa la stringa raw
      // (oppure parse con strtok)
      break;

    case MSG_PUNTI_FINALI:
      printf("Classifica finale: %s\n", data);
      break;

    case MSG_ERR:
      printf("Errore: %s\n", data);
      break;

    case MSG_OK:
      printf("Operazione completata con successo\n");
      break;

    case MSG_SERVER_SHUTDOWN:
      printf("Il server sta per chiudersi. Terminazione del client.\n");
      sleep(2);
      exit(0);
      break;
    }
  }
  
  // Chiudi il socket prima di terminare il thread
  close(sock);
  pthread_exit(NULL);

}
// Funzione per richiedere il tempo residuo al server
int richiedi_tempo_residuo(int sock)
{
  char msg_type = MSG_TEMPO_PARTITA;        // Tipo di messaggio per richiedere il tempo residuo
  write(sock, &msg_type, sizeof(msg_type)); // Invia la richiesta al server

  char response_type;           // Tipo di messaggio di risposta
  unsigned int response_length; // Lunghezza del messaggio di risposta
  int tempo_residuo;

  read(sock, &response_type, sizeof(response_type));     // Legge il tipo di messaggio di risposta
  read(sock, &response_length, sizeof(response_length)); // Legge la lunghezza del messaggio di risposta
  read(sock, &tempo_residuo, response_length);           // Legge il tempo residuo

  return tempo_residuo; // Ritorna il tempo residuo
}

void print_help()
{
  printf("Comandi disponibili:\n");
  printf("aiuto -> Mostra questo messaggio di aiuto\n");
  printf("registra_utente <nome_utente> -> Registra un nuovo utente (sono ammessi solo caratteri alfanumerici dell'alfabeto italiano)\n");
  printf("login_utente <nome_utente> -> Effettua il login con un utente esistente\n");
  printf("cancella_utente <nome_utente> -> Cancella un utente registrato\n");
  printf("matrice -> Richiede la matrice corrente (devi essere registrato)\n");
  printf("p <parola> -> Invia una parola al server (devi essere registrato)\n");
  printf("msg <testo_messaggio> -> Invia un messaggio sulla bacheca\n");
  printf("show-msg -> Mostra i messaggi sulla bacheca\n");
  printf("fine -> Esci dal gioco\n");
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr, "Utilizzo: %s <nome_server> <porta>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int retvalue;
  int sock;
  struct sockaddr_in server_address;
  char *server_name = argv[1];
  int port = atoi(argv[2]); // Ottiene la porta del server dalla riga di comando
  struct addrinfo hints, *res, *p;

  // Crea il socket del client
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Errore nella creazione del socket");
    exit(EXIT_FAILURE);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  SYSC(retvalue, getaddrinfo(server_name, NULL, &hints, &res), "Getaddrinfo error");

  for (p = res; p != NULL; p = p->ai_next)
  {
    if (p->ai_family == AF_INET)
    {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      memcpy(&server_address, ipv4, sizeof(struct sockaddr_in));
      break;
    }
  }

  freeaddrinfo(res);

  if (p == NULL)
  {
    fprintf(stderr, "Errore: non è stato trovato un indirizzo valido per il server\n");
    exit(1);
  }

  server_address.sin_port = htons(port);

  SYSC(retvalue, connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)), "Connect error");
  printf("Connesso al server\n");

  // Mostra il menu iniziale
  print_help();

  pthread_t recv_thread;
  if (pthread_create(&recv_thread, NULL, receiver_thread, (void *)&sock) != 0)
  {
    perror("pthread_create (receiver) failed");
    exit(EXIT_FAILURE);
  } 

  char command[1024];
  printf("[PROMPT PAROLIERE]--> ");
  while (fgets(command, sizeof(command), stdin) != NULL)
  {
    // Rimuove il carattere di nuova linea alla fine del comando
    command[strcspn(command, "\n")] = 0;

    // Suddivide il comando in parole
    char *token = strtok(command, " ");
    if (token == NULL)
    {
      printf("Comando non valido\n");
      printf("[PROMPT PAROLIERE]--> ");
      continue;
    }

    // Usa una variabile per il comando riconosciuto
    char command_type = 0;

    // Identifica il comando
    if (strcmp(token, "aiuto") == 0)
    {
      command_type = 'H';
    }
    else if (strcmp(token, "registra_utente") == 0)
    {
      token = strtok(NULL, " ");
      if (token != NULL)
      {
        command_type = 'R';
      }
    }
    else if (strcmp(token, "login_utente") == 0)
    {
      token = strtok(NULL, " ");
      if (token != NULL)
      {
        command_type = 'L';
      }
    }
    else if (strcmp(token, "cancella_utente") == 0)
    {
      token = strtok(NULL, " ");
      if (token != NULL)
      {
        command_type = 'D';
      }
    }
    else if (strcmp(token, "matrice") == 0)
    {
      command_type = 'M';
    }
    else if (strcmp(token, "p") == 0)
    {
      token = strtok(NULL, " ");
      if (token != NULL)
      {
        command_type = 'W';
      }
    }
    else if (strcmp(token, "msg") == 0)
    {
      token = strtok(NULL, "");
      if (token != NULL && strlen(token) > 0)
      {
        while (*token == ' ')
          token++;
        command_type = 'B';
      }
      else
      {
        printf("Uso corretto: msg <testo>\n");
      }
    }
    else if (strcmp(token, "show-msg") == 0)
    {
      command_type = 'S';
    }
    else if (strcmp(token, "fine") == 0)
    {
      command_type = 'Q';
    }

    switch (command_type)
    {
    case 'H':
      print_help();
      break;

    case 'R':
    {
      printf("Registrazione dell'utente %s\n", token);
      strncpy(my_name, token, sizeof(my_name) - 1);
      invia_messaggio(sock, MSG_REGISTRA_UTENTE, token);

      break;
    }
    case 'L':
    { 
      printf("Login dell'utente %s\n", token);
      strncpy(my_name, token, sizeof(my_name) - 1);
      invia_messaggio(sock, MSG_LOGIN_UTENTE, token);

      break;
    }

    case 'D':
    {
      printf("Cancellazione dell'utente %s\n", token);
      my_name[0] = '\0';
      invia_messaggio(sock, MSG_CANCELLA_UTENTE, token);

      break;
    }

    case 'M':
      if (!is_registered)
      {
        printf("Devi essere registrato per richiedere la matrice\n");
      }
      else
      {
        invia_messaggio(sock, MSG_MATRICE, "");
      }
      break;

    case 'W':
      if (!is_registered)
      {
        printf("Devi essere registrato per inviare una parola\n");
      }
      else
      {
        // Costruisci il payload come: "my_name\0parola"
        char payload[256];
        // token contiene la parola inserita (assicurati che non sia NULL)
        snprintf(payload, sizeof(payload), "%s|%s", my_name, token);

        printf("Nome|Parola: %s\n", payload);

        invia_messaggio(sock, MSG_PAROLA, payload);
      }
      break;

    case 'B':
      if(!is_registered)
      {
        printf("Devi essere registrato per inviare un messaggio\n");
      }
      else
      {
        char payload[256];
        snprintf(payload, sizeof(payload), "%s|%s", my_name, token);
        printf("Invio: '%s'\n", payload);
        invia_messaggio(sock, MSG_POST_BACHECA, payload);
      }
      break;

    case 'S':
      invia_messaggio(sock, MSG_SHOW_BACHECA, "");
      break;

    case 'Q':
      printf("Uscita dal gioco\n");
      close(sock);
      exit(0);

    default:
      printf("Comando non valido\n");
      break;
    }

    printf("[PROMPT PAROLIERE]--> ");
  }

  // Chiude il socket
  close(sock);

  return 0;
}
