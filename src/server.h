#ifndef _SERVER_H_
#define _SERVER_H_

#define _GNU_SOURCE

#include <log4c.h>
#include <errno.h>

#include "csapp.h"
#include "util.h"
#include "http.h"

#define MAX_CONNECTION_QUEUE 20

/* Struct to hold server-wide settings and variables */
typedef struct dirt_server {
    unsigned int port;
    char static_file_path[MAX_PATH_LENGTH];
    char dynamic_file_path[MAX_PATH_LENGTH];
    int socket;
    pthread_attr_t thread_attr; /* Attributes for receive threads */
} dirt_server;

/* Arguments for spawned receiver threads */
typedef struct {
    int incoming_socket;
    dirt_server* server;
} receive_args;

/* Initialize dirt_server struct server with values specified.
 *
 * Modifies *server.
 * Returns 0 if successful.
 */
int initialize_server(dirt_server* server);

/* Main thread for proxy server. Listens on the server socket and spawns
 * threads to handle new requests. Should never return.
 *
 * Requires server to be initialized with initialize_server.
 */
void run_server(dirt_server* server);

void shutdown_server(dirt_server* server);

#endif // _SERVER_H_
