#ifndef SOCKET_AUX
#define SOCKET_AUX
#include <stdint.h>

int sendall(int s, const char *buf, int *len);
int readall(int s, char *buf, int *len);

uint32_t readint(int s);
int sendint(int s, uint32_t num);

void *get_in_addr(struct sockaddr *sa);
int setup_server(const char * port, int * port_num);
int setup_client(const char * hostname, const char * port);
#endif
