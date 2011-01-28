#ifndef _SERVER_H_
#define _SERVER_H_

#define _GNU_SOURCE

#include <log4c.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "csapp.h"
#include "util.h"
#include "http.h"
#include "cgi.h"
#include "types.h"

#define MAX_CONNECTION_QUEUE 128

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

void register_cgi_handler(spade_server* server, const char* path,
        const char* handler_path);

void register_dirt_handler(spade_server* server, const char* path,
        const char* handler_path);

#endif // _SERVER_H_
