#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include "structs.h"

int post_bacheca(const char *user, const char *text);
int show_bacheca(char *out, size_t out_size);