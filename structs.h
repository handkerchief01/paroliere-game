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
#define MSG_USCITA 'U'
#define MSG_HELP 'H'
#define MSG_CLASSIFICA 'C'

#define BUFFERSIZE 1024

// Definizione della struttura Message che indica come deve essere fatto un messaggio da inviare
typedef struct
{
  char type;
  unsigned int length;
  char *data;
} Message;

// Struttura per memorizzare le informazioni degli utenti
typedef struct Utente
{
  char nome[100];      // Nome dell'utente
  int punteggio;       // Punteggio dell'utente
  struct Utente *next; // Puntatore al prossimo utente nella lista
} Utente;