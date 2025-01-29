void send_message(int sock, char type, const char *data);
void receive_message(int sock, char *type, int *size, char *data);

int is_italian_alnum(char c);