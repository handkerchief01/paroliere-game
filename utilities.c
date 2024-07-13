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