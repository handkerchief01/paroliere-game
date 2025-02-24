#include "matrice.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

Matrice array_matrici[MAX_MATRICI]; // tutte le matrici lette
int count_matrici = 0;              // quante righe/matrici abbiamo letto
int current_index = 0;              // indice della prossima matrice da usare

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

int get_next_matrice(Matrice *dest)
{
  if (count_matrici == 0 && current_index == 0) // quindi se non viene letto alcun file (matrice.txt)
  {
    genera_matrice_casuale(&mat_attuale);
    array_matrici[0] = mat_attuale;
    return 0;
  }

  // Copia la matrice dall'array in dest
  *dest = array_matrici[current_index];

  // Avanziamo l'indice in modo circolare (se vuoi riusare in loop)
  // o lo incrementi fino a fermarti all'ultima
  current_index = (current_index + 1) % count_matrici;
  return 0;
}

int leggi_tutte_le_matrici(const char *filename)
{
  FILE *f = fopen(filename, "r");
  if (!f)
  {
    perror("Errore apertura file matrici");
    return -1;
  }
  count_matrici = 0;

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), f))
  {
    // Stampiamo la riga grezza letta (debug)
    printf("Riga letta: '%s'\n", buffer);

    // Se la riga è vuota, skip
    if (buffer[0] == '\n' || buffer[0] == '\0')
    {
      printf(" -> Riga vuota, salto.\n");
      continue;
    }

    // Parse 16 token
    char *token = strtok(buffer, " \t\r\n");
    int tokenCount = 0;
    while (token && tokenCount < 16)
    {
      printf("   Token #%d: '%s'\n", tokenCount + 1, token); // debug
      strncpy(array_matrici[count_matrici].matrice[tokenCount / 4][tokenCount % 4],
              token, 3);
      array_matrici[count_matrici].matrice[tokenCount / 4][tokenCount % 4][3] = '\0';
      tokenCount++;
      token = strtok(NULL, " \t\r\n");
    }
    printf(" => Trovati %d token in questa riga.\n", tokenCount);

    // Se la riga ha 16 token validi, contiamo questa matrice
    if (tokenCount == 16)
    {
      count_matrici++;
      printf(" -> OK, matrice %d caricata.\n", count_matrici);
      if (count_matrici >= MAX_MATRICI)
      {
        printf("Raggiunto il limite di %d matrici.\n", MAX_MATRICI);
        break;
      }
    }
    else
    {
      printf(" -> Riga con tokenCount != 16, ignorata.\n");
    }
  }

  fclose(f);

  printf("Totale matrici caricate: %d\n", count_matrici);
  return 0;
}