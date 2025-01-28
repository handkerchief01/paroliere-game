#ifndef MATRICE_H // verifica se la libreria MATRICE_H è già stata inclusa
#define MATRICE_H

#define MATRIX_SIZE 4

// Definizione della struttura Matrice
typedef struct
{
  char matrice[MATRIX_SIZE][MATRIX_SIZE][3]; // Ogni cella può contenere una stringa di 2 caratteri più il terminatore nullo
} Matrice;

// Dichiarazione delle funzioni
void genera_matrice_casuale(Matrice *mat);
void stampa_matrice(const Matrice *mat);

#endif // MATRICE_H
