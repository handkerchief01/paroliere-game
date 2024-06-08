#include "matrice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Lettere possibili per la matrice casuale
const char *lettere_possibili[] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
    "Qu", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};

void genera_matrice_casuale(Matrice *mat)
{
  srand(time(NULL));

  int numero_lettere = sizeof(lettere_possibili) / sizeof(lettere_possibili[0]);

  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      int random_index = rand() % numero_lettere;
      strncpy(mat->matrice[i][j], lettere_possibili[random_index], 3);
    }
  }
}

int leggi_matrice_da_file(Matrice *mat, const char *filename)
{
  FILE *file = fopen(filename, "r");
  if (file == NULL)
  {
    perror("Errore nell'apertura del file");
    return -1;
  }

  char buffer[100]; // Buffer per leggere le righe del file
  int row = 0;

  while (fgets(buffer, sizeof(buffer), file) && row < MATRIX_SIZE)
  {
    char *token = strtok(buffer, " ");
    int col = 0;
    while (token != NULL && col < MATRIX_SIZE)
    {
      strncpy(mat->matrice[row][col], token, 3);
      token = strtok(NULL, " ");
      col++;
    }
    row++;
  }

  fclose(file);

  if (row != MATRIX_SIZE)
  {
    fprintf(stderr, "Errore: il file non contiene abbastanza righe.\n");
    return -1;
  }

  return 0;
}

void stampa_matrice(const Matrice *mat)
{
  for (int i = 0; i < MATRIX_SIZE; i++)
  {
    for (int j = 0; j < MATRIX_SIZE; j++)
    {
      printf("%s ", mat->matrice[i][j]);
    }
    printf("\n");
  }
}
