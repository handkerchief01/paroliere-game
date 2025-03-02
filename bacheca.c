#include "bacheca.h"

#define MAX_BACHECA 8
static BachecaMsg bacheca[MAX_BACHECA];
static int bacheca_count = 0; // quanti messaggi attualmente in bacheca (<= 8)
static int bacheca_index = 0; // indice circolare
pthread_mutex_t bacheca_mutex = PTHREAD_MUTEX_INITIALIZER;


int post_bacheca(const char *user, const char *text)
{
  // Controlla lunghezza di text
  if (strlen(text) > MAX_MSG_LEN)
  {
    return 0;
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

  return 1;
}

/**
 * Costruisce un CSV con i messaggi in ordine dal più vecchio al più nuovo.
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
