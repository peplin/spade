#ifndef _SERVER_H_
#define _SERVER_H_

#define _GNU_SOURCE

#include <log4c.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dlfcn.h>
#include <unistd.h>
#include <zmq.h>

#include "debug.h"
#include "csapp.h"
#include "util.h"
#include "http.h"
#include "cgi.h"
#include "dirt.h"
#include "clay.h"

#define MAX_CONNECTION_QUEUE 3000
#define ZMQ_THREAD_POOL_SIZE 4

/* Struct to hold server-wide settings and variables */
typedef struct spade_server {
    pthread_attr_t thread_attr; /* Attributes for receive threads */
    unsigned int port;
    char static_file_path[MAX_PATH_LENGTH];
    char cgi_file_path[MAX_PATH_LENGTH];
    char dirt_file_path[MAX_PATH_LENGTH];
    char hostname[MAX_HOSTNAME_LENGTH];
    int socket;
	int do_reverse_lookups;
    unsigned int cgi_handler_count;
    cgi_handler cgi_handlers[MAX_HANDLERS];
    unsigned int dirt_handler_count;
    dirt_handler dirt_handlers[MAX_HANDLERS];
    unsigned int clay_handler_count;
    clay_handler clay_handlers[MAX_HANDLERS];
    void* zmq_context;
} spade_server;


/* Arguments for spawned receiver threads */
typedef struct {
    spade_server* server;
    int incoming_socket;
    struct sockaddr_in client_address;
} receive_args;

/* Initialize spade_server struct server with values specified.
 *
 * Modifies *server.
 * Returns 0 if successful.
 */
int initialize_server(spade_server* server);

/* Main thread for proxy server. Listens on the server socket and spawns
 * threads to handle new requests. Should never return.
 *
 * Requires server to be initialized with initialize_server.
 */
void run_server(spade_server* server);

void shutdown_server(spade_server* server);

int register_cgi_handler(spade_server* server, const char* path,
        const char* handler_path);

int register_dirt_handler(spade_server* server, const char* path,
        const char* handler_path, const char* library);

int register_clay_handler(spade_server* server, const char* path,
        const char* endpoint);

#endif // _SERVER_H_
