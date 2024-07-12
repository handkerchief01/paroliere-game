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
#include "structs.h"

// Funzione per inviare un messaggio
void send_message(int sock, char type, const char *data)
{
  unsigned int length = strlen(data);

  printf("Invio al server: Type=%c, Length=%u, Data=%s\n", type, length, data);

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

void print_help()
{
  printf("Comandi disponibili:\n");
  printf("aiuto -> Mostra questo messaggio di aiuto\n");
  printf("registra utente <nome_utente> -> Registra un nuovo utente\n");
  printf("matrice -> Richiede la matrice corrente (devi essere registrato)\n");
  printf("p <parola> -> Invia una parola al server (devi essere registrato)\n");
  printf("fine -> Esci dal gioco\n");
}

// Funzione per stampare la matrice
void print_matrice(const char *data)
{
  char matrice[4][4][4];
  int index = 0;

  // Parsing della stringa ricevuta per ottenere la matrice
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      strncpy(matrice[i][j], &data[index], 3);
      matrice[i][j][3] = '\0';
      index += 3;
    }
  }

  // Parsing del tempo residuo
  int tempo_residuo = atoi(&data[index]);

  // Stampa della matrice
  printf("Matrice:\n");
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      printf("%s ", matrice[i][j]);
    }
    printf("\n");
  }
  printf("Tempo residuo: %d secondi\n", tempo_residuo);
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
  int is_registered = 0; // Variabile per tracciare se l'utente è registrato

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
    }

    // Usa una variabile per il comando riconosciuto
    char command_type = 0;

    // Identifica il comando
    if (strcmp(token, "aiuto") == 0)
    {
      command_type = 'H';
    }
    else if (strcmp(token, "registra") == 0)
    {
      token = strtok(NULL, " ");
      if (token != NULL && strcmp(token, "utente") == 0)
      {
        token = strtok(NULL, " ");
        if (token != NULL)
        {
          command_type = 'R';
        }
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
      printf("Registrazione dell'utente %s\n", token);
      send_message(sock, MSG_REGISTRA_UTENTE, token);
      
      // Riceve la risposta dal server
      {
        char type;
        unsigned int length;
        char data[1024];
        receive_message(sock, &type, &length, data);

        printf("Ricevuto dal server: Type=%c, Length=%u, Data=%s\n", type, length, data);

        if (type == MSG_OK)
        {
          printf("Registrazione avvenuta con successo\n");
          is_registered = 1; // Imposta lo stato come registrato
        }
        else if (type == MSG_ERR)
        {
          printf("Errore: %s\n", data);
        }
        else
        {
          printf("Tipo di messaggio sconosciuto ricevuto: %c\n", type);
        }
      }
      break;

    case 'M':
      if (!is_registered)
      {
        printf("Devi essere registrato per richiedere la matrice\n");
      }
      else
      {
        send_message(sock, MSG_MATRICE, "");

        // Riceve la risposta dal server
        {
          char type;
          unsigned int length;
          char data[1024];
          receive_message(sock, &type, &length, data);

          printf("Ricevuto dal server: Type=%c, Length=%u, Data=%s\n", type, length, data);

          if (type == MSG_MATRICE)
          {
            print_matrice(data);
          }
          else if (type == MSG_ERR)
          {
            printf("Errore: %s\n", data);
          }
          else
          {
            printf("Tipo di messaggio sconosciuto ricevuto: %c\n", type);
          }
        }
      }
      break;

    case 'W':
      if (!is_registered)
      {
        printf("Devi essere registrato per inviare una parola\n");
      }
      else
      {
        send_message(sock, MSG_PAROLA, token);

        // Riceve la risposta dal server
        {
          char type;
          unsigned int length;
          char data[1024];
          receive_message(sock, &type, &length, data);

          printf("Ricevuto dal server: Type=%c, Length=%u, Data=%s\n", type, length, data);

          if (type == MSG_PUNTI_PAROLA)
          {
            printf("Punti parola: %s\n", data);
          }
          else if (type == MSG_ERR)
          {
            printf("Errore: %s\n", data);
          }
          else
          {
            printf("Tipo di messaggio sconosciuto ricevuto: %c\n", type);
          }
        }
      }
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
