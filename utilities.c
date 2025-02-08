#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

//! per stampare la struttura correttamente. Da mettere nel main
//! printTrie(dictionaryRoot, "", 1);
// void printTrie(TrieNode *node, const char *prefix, int isLast)
// {
//   // Se il nodo corrente non è la radice, stampa il ramo
//   if (prefix != NULL && strlen(prefix) > 0)
//   {
//     printf("%s", prefix);
//     if (isLast)
//       printf("└── ");
//     else
//       printf("├── ");
//   }

//   if (prefix == NULL || strlen(prefix) == 0)
//     printf("ROOT\n");
//   else
//   {
//     if (node->isEndOfWord)
//       printf("(end)\n");
//     else
//       printf("\n");
//   }

//   int childCount = 0;
//   for (int i = 0; i < ALPHABET_SIZE; i++)
//   {
//     if (node->children[i] != NULL)
//       childCount++;
//   }

//   int printed = 0;
//   char newPrefix[256];
//   for (int i = 0; i < ALPHABET_SIZE; i++)
//   {
//     if (node->children[i] != NULL)
//     {
//       printed++;
//       if (prefix != NULL && strlen(prefix) > 0)
//         strcpy(newPrefix, prefix);
//       else
//         newPrefix[0] = '\0';

//       if (isLast)
//         strcat(newPrefix, "    ");
//       else
//         strcat(newPrefix, "│   ");

//       char label[4];
//       snprintf(label, sizeof(label), "%c", 'A' + i);

//       char endStr[8] = "";
//       if (node->children[i]->isEndOfWord)
//         strcpy(endStr, " (end)");

//       printf("%s%s%s\n", newPrefix, label, endStr);

//       printTrie(node->children[i], newPrefix, (printed == childCount));
//     }
//   }
// }
