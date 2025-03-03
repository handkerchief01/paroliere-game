#ifndef MATRICE_H
#define MATRICE_H

#include <string.h>
#include "structs.h"


#define MAX_MATRICI 100 // numero massimo di righe/matrici nel file

extern Matrice mat_attuale;

void genera_matrice_casuale(Matrice *mat);
void matrice_to_string(const Matrice *mat, char *out, size_t size);
int get_next_matrice(Matrice *dest);
int leggi_tutte_le_matrici(const char *filename);

#endif
