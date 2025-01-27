#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "structs.h"
#include "macros.h"
#include "utilities.h"

void send_message(int sock, char type, const char *data){
  int ret;
  unsigned int length = strlen(data);

  SYSC(ret, write(sock, (void *)&type, sizeof(char)), "Write error");
  SYSC(ret, write(sock, (void *)&length, sizeof(unsigned int)), "Write error");
  SYSC(ret, write(sock, (void *)(data), sizeof(char) * length), "Write error");
}

void receive_message(int sock, char *type, unsigned int *length, char *data){
  int ret;
  
  SYSC(ret, read(sock, (void *)type, sizeof(char)), "Read error");
  SYSC(ret, read(sock, (void *)length, sizeof(unsigned int)), "Read error");
  SYSC(ret, read(sock, (void *)data, sizeof(char) * (*length)), "Read error");
  data[*length] = '\0';
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
