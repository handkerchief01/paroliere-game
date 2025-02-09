#ifndef TRIE_H
#define TRIE_H

#include <stddef.h>

#define ALPHABET_SIZE 26

// Definizione del nodo del Trie
typedef struct TrieNode
{
  int isEndOfWord; // 1 se il nodo rappresenta la fine di una parola
  struct TrieNode *children[ALPHABET_SIZE];
} TrieNode;

// Crea e restituisce un nuovo nodo del Trie
TrieNode *createTrieNode(void);

// Inserisce la parola nel Trie
void insertWord(TrieNode *root, const char *word);

// Cerca la parola nel Trie (ritorna 1 se trovata, 0 altrimenti)
int searchWord(TrieNode *root, const char *word);

// Libera la memoria allocata dal Trie
void freeTrie(TrieNode *root);

// Carica il dizionario dal file specificato nel Trie
TrieNode *loadDictionary(const char *filename);

#endif
