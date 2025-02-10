#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include "structs.h"
#include "macros.h"
#include "utilities.h"

void send_message(int sock, char type, const char *data)
{
  int ret;
  // Calcoliamo la lunghezza del payload
  int size = (data == NULL) ? 0 : strlen(data);

  // 1) Inviamo la dimensione (int)
  SYSC(ret, write(sock, &size, sizeof(int)), "Write error: size");

  // 2) Inviamo il tipo (char)
  SYSC(ret, write(sock, &type, sizeof(char)), "Write error: type");

  // 3) Inviamo il payload (size byte, se > 0)
  if (size > 0)
  {
    SYSC(ret, write(sock, data, size), "Write error: payload");
  }
}

void receive_message(int sock, char *type, int *size, char *data)
{
  int ret;

  // 1) Leggiamo la dimensione (int)
  SYSC(ret, read(sock, size, sizeof(int)), "Read error: size");
  if (ret <= 0)
  {
    // Se ret=0, connessione chiusa; se <0, errore
    // Potresti gestire qui la chiusura o eccezione
  }

  // 2) Leggiamo il tipo (char)
  SYSC(ret, read(sock, type, sizeof(char)), "Read error: type");
  if (ret <= 0)
  {
    // gestione errore/chiusura
  }

  // 3) Leggiamo il payload (se size > 0)
  if (*size > 0)
  {
    SYSC(ret, read(sock, data, *size), "Read error: payload");
    data[*size] = '\0'; // terminatore di stringa
  }
  else
  {
    // payload vuoto
    data[0] = '\0';
  }
}

int is_italian_alnum(char c)
{
  // Se è una cifra, lo ammettiamo
  if (c >= '0' && c <= '9')
  {
    return 1;
  }

  // Convertiamo in maiuscolo per il confronto
  unsigned char uc = (unsigned char)c;
  uc = (unsigned char)toupper(uc);

  // Alfabeto italiano tradizionale (21 lettere)
  // Qui escludiamo J, K, W, X, Y
  // Quindi consideriamo validi: A B C D E F G H I L M N O P Q R S T U V Z
  const char *alfabeto_italiano = "ABCDEFGHILMNOPQRSTUVZ";

  // Controlliamo se uc è una di queste lettere
  if (strchr(alfabeto_italiano, uc) != NULL)
  {
    return 1;
  }

  return 0;
}

void receive_and_check(int sock, char *type, int *size, char *data)
{
  receive_message(sock, type, size, data);
  if (*type == MSG_SERVER_SHUTDOWN)
  {
    printf("Il server sta per chiudersi. Terminazione del client.\n");
    close(sock);
    exit(0);
  }
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
  strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S]", localtime(&now));
  fprintf(logFile, "%s ", timeStr);

  va_list args;
  va_start(args, format);
  vfprintf(logFile, format, args);
  va_end(args);

  fprintf(logFile, "\n");
  fflush(logFile);
  pthread_mutex_unlock(&logMutex);
}
