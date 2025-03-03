#include <pthread.h>
#include "structs.h" 
#include "trie.h"  
#include "macros.h"
#include "matrice.h"

extern Utente *utenti_head;
extern pthread_mutex_t utenti_mutex;

extern Matrice mat_attuale;
extern TrieNode *dictionaryRoot;

int registra_utente(int sock, const char *nome);
int login_utente(int sock, const char *nome);
int cancella_utente(const char *nome);

int parola_in_matrice(const Matrice *mat, const char *parola);
int verifica_parola(const char *parola, const Matrice *mat);
int dfs_parola(const Matrice *mat, int i, int j, const char *parola, int index, int visited[MATRIX_SIZE][MATRIX_SIZE]);

int calcola_punteggio(const char *parola);

/* Funzione di confronto per qsort */
int cmp_utente_punteggio_desc(const void *a, const void *b);

void aggiorna_punteggio(const char *nome, int punteggio);
void build_classifica_csv(char *out, size_t out_size);

void handle_parola(int client_socket, const char *nome, const char *parola);
