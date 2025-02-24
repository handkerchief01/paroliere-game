#ifndef MATRICE_H // verifica se la libreria MATRICE_H è già stata inclusa
#define MATRICE_H

#include <string.h>

#define MATRIX_SIZE 4
#define MAX_MATRICI 100 // numero massimo di righe/matrici nel file
// Definizione della struttura Matrice
typedef struct
{
  char matrice[MATRIX_SIZE][MATRIX_SIZE][3]; // Ogni cella può contenere una stringa di 2 caratteri più il terminatore nullo
} Matrice;

extern Matrice mat_attuale;

// Dichiarazione delle funzioni
void genera_matrice_casuale(Matrice *mat);
void matrice_to_string(const Matrice *mat, char *out, size_t size);
int get_next_matrice(Matrice *dest);
int leggi_tutte_le_matrici(const char *filename);

#endif // MATRICE_H
