#ifndef TRIE_H
#define TRIE_H

#include <stddef.h>

#include "structs.h"

TrieNode *createTrieNode(void);

void insertWord(TrieNode *root, const char *word);

int searchWord(TrieNode *root, const char *word);

void freeTrie(TrieNode *root);

TrieNode *loadDictionary(const char *filename);

#endif
