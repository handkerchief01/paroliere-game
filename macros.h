#include <errno.h>
#include <errno.h>
#undef SYSC
#define SYSC(v, c, m)                                                   \
  if ((v = c) == -1)                                                    \
  {                                                                     \
    if (errno == EPIPE)                                                 \
    {                                                                   \
      pthread_exit(NULL);                                               \
    }                                                                   \
    else if (errno == EAGAIN || errno == EWOULDBLOCK)                   \
    {                                                                   \
      /* Timeout o socket non pronto => restituiamo -2 al chiamante. */ \
      v = -2;                                                           \
    }                                                                   \
    else                                                                \
    {                                                                   \
      perror(m);                                                        \
      pthread_exit(NULL);                                               \
    }                                                                   \
  }

#define SYSCN(v, c, m)                                                  \
  if ((v = c) == NULL)                                                  \
  {                                                                     \
    if (errno == EPIPE)                                                 \
    {                                                                   \
      pthread_exit(NULL);                                               \
    }   \
    else if (errno == EAGAIN || errno == EWOULDBLOCK)                   \
    {                                                                   \
      /* Timeout o socket non pronto => restituiamo -2 al chiamante. */ \
      v = -2;                                                           \
    }                                                                   \
    else                                                                \
    {                                                                   \
      perror(m);                                                        \
      pthread_exit(NULL);                                               \
    }                                                                   \
  }
