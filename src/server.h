#ifndef _SERVER_H_
#define _SERVER_H_

#define _GNU_SOURCE

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

void echo_request(http_request* request, int socket, dirt_server* server);
int initialize_server(dirt_server* server, unsigned int port, int echo,
        int verbosity);
void run_server(dirt_server* server);

#endif // _SERVER_H_
