#ifndef _SERVER_H_
#define _SERVER_H_

#define _GNU_SOURCE

#include <log4c.h>

#include "csapp.h"
#include "util.h"
#include "http.h"

#define MAX_CONNECTION_QUEUE 20

/* Struct to hold server-wide settings and variables */
typedef struct dirt_server {
    unsigned int port;
    int socket;
    int echo; /* Option to echo the processed request back to client */
    /* Verbosity:
     * 0: No output
     * 1: Only errors
     * 2: Errors, requests
     * 3: Errors, requests, responses, caching information
     */
    int verbosity; 
    pthread_attr_t thread_attr; /* Attributes for receive threads */
    pthread_mutex_t stdout_mutex; /* For non-interleaved output */
} dirt_server;

/* Arguments for spawned receiver threads */
typedef struct {
    int incoming_socket;
    dirt_server* server;
} receive_args;

/* Echo the processes request back to the client at socket. Used for testing. */
void echo_request(http_request* request, int socket, dirt_server* server);

/* Initialize dirt_server struct server with values specified.
 *
 * Modifies *server.
 * Returns 0 if successful.
 */
int initialize_server(dirt_server* server, unsigned int port, int echo,
        int verbosity);

/* Main thread for proxy server. Listens on the server socket and spawns
 * threads to handle new requests. Should never return.
 *
 * Requires server to be initialized with initialize_server.
 */
void run_server(dirt_server* server);

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
        char *longmsg);

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
void serve_dynamic(int fd, char *filename, char *cgiargs);

/*
 * serve_static - copy a file back to the client
 */
void serve_static(int fd, char *filename, int filesize);

#endif // _SERVER_H_
