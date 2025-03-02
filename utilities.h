void invia_messaggio(int sock, char type, const char *data);
int ricevi_messaggio(int sock, char *type, int *size, char *data);

void log_event(const char *format, ...);