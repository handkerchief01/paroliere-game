#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

#include "structs.h"
#include "macros.h"
#include "utilities.h"

void invia_messaggio(int sock, char type, const char *data)
{
  int ret;
  // Calcoliamo la lunghezza del payload
  int length = (data == NULL) ? 0 : strlen(data);

  // 1) Inviamo la dimensione (int)
  SYSC(ret, write(sock, &length, sizeof(int)), "Errore in scrittura: length");

  // 2) Inviamo il tipo (char)
  SYSC(ret, write(sock, &type, sizeof(char)), "Errore in scrittura: type");

  // 3) Inviamo il payload (length byte, se > 0)
  if (length > 0)
  {
    SYSC(ret, write(sock, data, length), "Errore in scrittura: data");
  }
}

int ricevi_messaggio(int sock, char *type, int *length, char *data)
{
  int ret;

  SYSC(ret, read(sock, length, sizeof(int)), "Errore in lettura: length");
  if (ret == 0)
  {
    // EOF (il client ha chiuso la connessione in modo ordinato)
    return 0;
  }
  else if (ret == -2)
  {
    // EAGAIN/EWOULDBLOCK => timeout
    errno = EAGAIN;
    return -1;
  }
  // Se ret > 0, la lettura è andata avanti.

  // 2) Leggiamo il tipo (char)
  SYSC(ret, read(sock, type, sizeof(char)), "Errore in lettura: type");
  if (ret == 0)
    return 0;
  else if (ret == -2)
  {
    errno = EAGAIN;
    return -1;
  }

  // 3) Leggiamo il payload
  if (*length > 0)
  {
    SYSC(ret, read(sock, data, *length), "Errore in lettura: data");
    if (ret == 0)
      return 0;
    else if (ret == -2)
    {
      errno = EAGAIN;
      return -1;
    }
    data[*length] = '\0';
  }
  else
  {
    data[0] = '\0';
  }

  return 1;
}

FILE *logFile = NULL;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

void log_event(const char *format, ...)
{
  pthread_mutex_lock(&logMutex);
  if (logFile == NULL)
  {
    logFile = fopen("log.txt", "a");
    if (logFile == NULL)
    {
      perror("Errore apertura file log");
      pthread_mutex_unlock(&logMutex);
      return;
    }
  }
  time_t now = time(NULL);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "[%d-%m-%Y %H:%M:%S]", localtime(&now));
  fprintf(logFile, "%s ", timeStr);

  va_list args; // per gestire gli argomenti variabili
  va_start(args, format); // punta al primo elemento della lista di variabili
  vfprintf(logFile, format, args); // scrive sul file di log
  va_end(args); // ripulisce la lista

  fprintf(logFile, "\n");
  fflush(logFile);
  pthread_mutex_unlock(&logMutex);
}
