#ifndef STRUCTS_H
#define STRUCTS_H

#include <pthread.h>

// Definizioni di macro
#define MSG_OK 'K'
#define MSG_ERR 'E'
#define MSG_REGISTRA_UTENTE 'R'
#define MSG_MATRICE 'M'
#define MSG_TEMPO_PARTITA 'T'
#define MSG_TEMPO_ATTESA 'A'
#define MSG_PAROLA 'W'
#define MSG_PUNTI_FINALI 'F'
#define MSG_PUNTI_PAROLA 'P'
#define MSG_SERVER_SHUTDOWN 'B'
#define MSG_USCITA 'U'
#define MSG_CLASSIFICA 'C'
#define MSG_LOGIN_UTENTE 'L'
#define MSG_CANCELLA_UTENTE 'D'
#define MSG_POST_BACHECA 'H'
#define MSG_SHOW_BACHECA 'S'
#define MSG_PUNTI_FINALI 'F'
#define MAX_PAROLE 100
#define MAX_LEN_PAROLA 32
#define MAX_MSG_LEN 128

// Struttura per memorizzare le informazioni degli utenti
typedef struct Utente
{
  char nome[20];      // Nome dell'utente
  int punteggio;       // Punteggio dell'utente
  char parole_usate[MAX_PAROLE][MAX_LEN_PAROLA]; // Array di parole usate
  int num_parole; // Numero di parole usate
  struct Utente *next; // Puntatore al prossimo utente nella lista
} Utente;

typedef struct ClientNode
{
  int sock;
  struct ClientNode *next;
} ClientNode;

typedef struct
{
  char user[20];
  char text[MAX_MSG_LEN + 1];
} BachecaMsg;

#endif