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

// Struttura per memorizzare le informazioni degli utenti
typedef struct Utente
{
  char nome[10];      // Nome dell'utente
  int punteggio;       // Punteggio dell'utente
  struct Utente *next; // Puntatore al prossimo utente nella lista
} Utente;