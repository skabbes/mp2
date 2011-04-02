#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "socket.h"

#define BACKLOG 20

/*
 * Shamelessly taken from Beej's Guide For Network Programming
 * http://beej.us/guide/bgnet/output/html/multipage/advanced.html#sendall
 */
int sendall(int s, const char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
} 

// read an integer from a socket
uint32_t readint(int s){
    uint32_t size;
    int amount_to_send = sizeof(size);
    readall(s, (char *)(&size), &amount_to_send);
    size = ntohl(size);
    return size;
}

// send an integer over a socket
int sendint(int s, uint32_t number){
    uint32_t size = htonl(number);
    int amount_to_send = sizeof(size);
    return sendall(s, (char *)(&size), &amount_to_send);
}

// wrapper function for recv which automatically loops to real the entire data
int readall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've read 
    int bytesleft = *len; // how many we have left to read 
    int n;

    while(total < *len) {
        n = recv(s, buf+total, bytesleft, 0);
        if (n == 0 || n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
} 


/*
 * Shamelessly taken from Beej's Guide For Network Programming
 * http://beej.us/guide/bgnet/output/html/multipage/clientserver.html
 */
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*
 * Shamelessly taken from Beej's Guide For Network Programming
 * http://beej.us/guide/bgnet/output/html/multipage/clientserver.html
 */
int setup_server(const char * port, int * port_num){
	int server;  // listen on socket server
	struct addrinfo hints, *servinfo, *p;
	int yes=1;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv) );
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((server= socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(server, p->ai_addr, p->ai_addrlen) == -1) {
			close(server);
			perror("server: bind");
			continue;
		}

        if( port_num && p->ai_family == AF_INET ){
            struct sockaddr_in sin;
            socklen_t len = sizeof(sin);
            getsockname(server, (struct sockaddr *)&sin, &len);
            *port_num = ntohs( sin.sin_port ); 
        }
        else if( port_num && p->ai_family == AF_INET6 ){
            struct sockaddr_in6 sin6;
            socklen_t len = sizeof(sin6);
            getsockname(server, (struct sockaddr *)&sin6, &len);
            *port_num = ntohs( sin6.sin6_port ); 
        }

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(server, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
   return server;
}

/*
 * Shamelessly taken from Beej's Guide For Network Programming
 * http://beej.us/guide/bgnet/output/html/multipage/clientserver.html
 */
int setup_client(const char * hostname, const char * port){
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	//printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
	return sockfd;
}
