void invia_messaggio(int sock, char type, const char *data);
int ricevi_messaggio(int sock, char *type, int *size, char *data);

int is_italian_alnum(char c);

void log_event(const char *format, ...);