#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

#include "utilities.h"
#include "server_utilities.h"

// Funzione per registrare un utente nella lista
int registra_utente(const char *nome)
{
  printf("Registrazione utente: %s\n", nome);

  pthread_mutex_lock(&utenti_mutex);

  // Verifichiamo la lunghezza minima/massima
  if (strlen(nome) == 0 || strlen(nome) > 20)
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

int login_utente(const char *nome)
{
  int found = 0;
  pthread_mutex_lock(&utenti_mutex);
  Utente *curr = utenti_head;
  while (curr != NULL)
  {
    if (strcmp(curr->nome, nome) == 0)
    {
      found = 1;
      break;
    }
    curr = curr->next;
  }
  pthread_mutex_unlock(&utenti_mutex);
  return found;
}

int cancella_utente(const char *nome)
{
  int found = 0;
  pthread_mutex_lock(&utenti_mutex);
  Utente *curr = utenti_head;
  Utente *prev = NULL;
  while (curr != NULL)
  {
    if (strcmp(curr->nome, nome) == 0)
    {
      // Rimuovi curr dalla lista
      if (prev == NULL)
      {
        utenti_head = curr->next;
      }
      else
      {
        prev->next = curr->next;
      }
      free(curr);
      found = 1;
      break;
    }
    prev = curr;
    curr = curr->next;
  }
  pthread_mutex_unlock(&utenti_mutex);
  return found;
}

int parola_in_matrice(const Matrice *mat, const char *parola)
{
  int visited[MATRIX_SIZE][MATRIX_SIZE] = {0};
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      if (dfs_parola(mat, i, j, parola, 0, visited))
        return 1;
    }
  }
  return 0;
}

int verifica_parola(const char *parola, const Matrice *mat)
{
  // 2. Verifica se la parola può essere formata nella matrice
  if (!parola_in_matrice(mat, parola))
  {
    printf("Parola non presente nella matrice\n");
    return 0;
  }

  // 3. Verifica se la parola è presente nel dizionario
  if (!searchWord(dictionaryRoot, parola))
  {
    printf("Parola non presente nel dizionario\n");
    return 0;
  }

  return 1;
}

int dfs_parola(const Matrice *mat, int i, int j, const char *parola, int index, int visited[MATRIX_SIZE][MATRIX_SIZE])
{
  int len = strlen(parola);
  if (index == len)
    return 1; // Tutti i caratteri sono stati trovati

  // Controlla i limiti della matrice
  if (i < 0 || i >= MATRIX_SIZE || j < 0 || j >= MATRIX_SIZE)
    return 0;
  if (visited[i][j])
    return 0;

  const char *cell = mat->matrice[i][j];
  int cellLen = strlen(cell);

  // Confronta, in modo case-insensitive, la parte della parola con il contenuto della cella
  if (strncasecmp(parola + index, cell, cellLen) != 0)
    return 0;

  // Se la cella contiene "Qu", consideriamo l'avanzamento di 1 lettera, altrimenti cellLen
  int advance = (strcasecmp(cell, "Qu") == 0) ? 1 : cellLen;

  visited[i][j] = 1;
  int found = 0;
  for (int di = -1; di <= 1 && !found; di++)
  {
    for (int dj = -1; dj <= 1 && !found; dj++)
    {
      if (di == 0 && dj == 0)
        continue;
      if (dfs_parola(mat, i + di, j + dj, parola, index + advance, visited))
        found = 1;
    }
  }
  visited[i][j] = 0;
  return found;
}

int calcola_punteggio(const char *parola)
{
  printf("Calcolo punteggio per parola");
  return (int)strlen(parola);
}

// Funzione di confronto per qsort
int cmp_utente_punteggio_desc(const void *a, const void *b)
{
  Utente *ua = *(Utente **)a;
  Utente *ub = *(Utente **)b;
  return (ub->punteggio - ua->punteggio);
  // se ub > ua => positivo => ub prima di ua => ordinamento decrescente
}

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

void build_classifica_csv(char *out, size_t out_size)
{
  pthread_mutex_lock(&utenti_mutex);

  // Conta quanti utenti
  int count = 0;
  for (Utente *u = utenti_head; u != NULL; u = u->next)
  {
    count++;
  }
  if (count == 0)
  {
    pthread_mutex_unlock(&utenti_mutex);
    // Nessun utente => CSV vuoto
    out[0] = '\0';
    return;
  }

  // Alloca array di puntatori
  Utente **array = malloc(count * sizeof(Utente *));
  if (!array)
  {
    pthread_mutex_unlock(&utenti_mutex);
    out[0] = '\0';
    return;
  }
  int i = 0;
  for (Utente *u = utenti_head; u != NULL; u = u->next)
  {
    array[i++] = u;
  }

  // Sblocchiamo il mutex prima dell'ordinamento
  pthread_mutex_unlock(&utenti_mutex);

  // Ordina in base al punteggio decrescente
  qsort(array, count, sizeof(Utente *), cmp_utente_punteggio_desc);

  // Costruiamo lo stile multi-riga
  // Prima riga di intestazione
  size_t used = 0;
  int written = snprintf(out, out_size,
                         "\"Utente\",\"Punteggio\"\n");
  if (written < 0)
  {
    free(array);
    out[0] = '\0';
    return;
  }
  if ((size_t)written >= out_size)
  {
    // overflow
    free(array);
    out[0] = '\0';
    return;
  }
  used = written;

  // Ora ogni utente su una riga: "nome","punteggio"
  for (int k = 0; k < count; k++)
  {
    written = snprintf(out + used, out_size - used,
                       "\"%s\",%d\n",
                       array[k]->nome,
                       array[k]->punteggio);
    if (written < 0)
    {
      break;
    }
    if ((size_t)written >= (out_size - used))
    {
      // overflow
      break;
    }
    used += written;
  }

  if (count > 0)
  {
    written = snprintf(out + used, out_size - used,
                       "\"Vincitore\",\"%s\". Una nuova partita inizierà tra un minuto\n",
                       array[0]->nome);
    if (written > 0 && (size_t)written < (out_size - used))
    {
      used += written;
    }
  }

  free(array);
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
    snprintf(response_data, sizeof(response_data), "Parola già inserita, punteggio parola: %d", 0);
    invia_messaggio(client_socket, response_type, response_data);
    return;
  }

  // Verifica correttezza della parola (controlla dizionario, presenza nella matrice, lunghezza, ecc.)
  if (!verifica_parola(parola, &mat_attuale))
  {
    response_type = MSG_ERR;
    strcpy(response_data, "Parola non valida");
    invia_messaggio(client_socket, response_type, response_data);
    return;
  }

  // Se la parola è corretta e non è duplicata:
  int punteggio = calcola_punteggio(parola);
  // Aggiorna il punteggio dell'utente
  aggiorna_punteggio(nome, punteggio);

  log_event("Parola proposta da %s: %s, Punteggio assegnato: %d", nome, parola, punteggio);

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
  invia_messaggio(client_socket, response_type, response_data);
}