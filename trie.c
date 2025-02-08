#include "trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

TrieNode *createTrieNode(void)
{
  TrieNode *node = malloc(sizeof(TrieNode));
  if (!node)
  {
    perror("Errore allocazione TrieNode");
    exit(EXIT_FAILURE);
  }
  node->isEndOfWord = 0;
  for (int i = 0; i < ALPHABET_SIZE; i++)
    node->children[i] = NULL;
  return node;
}

void insertWord(TrieNode *root, const char *word)
{
  TrieNode *current = root;
  for (int i = 0; word[i] != '\0'; i++)
  {
    char c = tolower((unsigned char)word[i]);
    if (c < 'a' || c > 'z') // Salta caratteri non alfabetici
      continue;
    int index = c - 'a';
    if (current->children[index] == NULL)
      current->children[index] = createTrieNode();
    current = current->children[index];
  }
  current->isEndOfWord = 1;
}

int searchWord(TrieNode *root, const char *word)
{
  TrieNode *current = root;
  for (int i = 0; word[i] != '\0'; i++)
  {
    char c = toupper((unsigned char)word[i]);
    if (c < 'A' || c > 'Z')
      continue;
    // Se trovi 'Q', controlla se il successivo è 'U'
    if (c == 'Q' && toupper((unsigned char)word[i + 1]) == 'U')
    {
      // Incrementa l'indice per saltare la 'U'
      i++;
    }
    int index = c - 'A';
    if (current->children[index] == NULL)
      return 0;
    current = current->children[index];
  }
  return current != NULL && current->isEndOfWord;
}

void freeTrie(TrieNode *root)
{
  if (root == NULL)
    return;
  for (int i = 0; i < ALPHABET_SIZE; i++)
  {
    freeTrie(root->children[i]);
  }
  free(root);
}

TrieNode *loadDictionary(const char *filename)
{
  FILE *f = fopen(filename, "r");
  if (!f)
  {
    perror("Errore apertura dizionario");
    return NULL;
  }
  TrieNode *root = createTrieNode();
  char buffer[128];
  while (fgets(buffer, sizeof(buffer), f))
  {
    buffer[strcspn(buffer, "\n")] = '\0'; // Rimuove newline
    if (strlen(buffer) > 0)
      insertWord(root, buffer);
  }
  fclose(f);
  return root;
}
