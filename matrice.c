#include "matrice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *lettere_possibili[] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
    "Qu", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};

// Genera una matrice casuale 4x4
void genera_matrice_casuale(Matrice *mat)
{
  srand(time(NULL));
  int numero_lettere = sizeof(lettere_possibili) / sizeof(lettere_possibili[0]);

  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      int random_index = rand() % numero_lettere;
      // Copio al massimo 3 caratteri (es. "Qu" + terminatore)
      strncpy(mat->matrice[i][j], lettere_possibili[random_index], 3);
      mat->matrice[i][j][3] = '\0'; // Assicuro terminazione
    }
  }
}

// Converte la matrice in una stringa (testuale) da inviare al client
void matrice_to_string(const Matrice *mat, char *dest, size_t size)
{
  memset(dest, 0, size);
  int pos = 0;

  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      // "pos" tiene traccia di dove stiamo scrivendo
      // snprintf evita sforamenti se la stringa è lunga
      pos += snprintf(dest + pos, size - pos, "%s ", mat->matrice[i][j]);
    }
    pos += snprintf(dest + pos, size - pos, "\n");
  }
}
