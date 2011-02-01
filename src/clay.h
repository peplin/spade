#ifndef _CLAY_H_
#define _CLAY_H_

#define _GNU_SOURCE

#include <zmq.h>

#include "http.h"
#include "constants.h"

#define MAX_CLAY_PARAMETER_LENGTH 255
#define MAX_ENDPOINT 255

struct spade_server;

typedef struct {
    char server_software[MAX_CLAY_PARAMETER_LENGTH];
    char server_name[MAX_CLAY_PARAMETER_LENGTH];
    char gateway_interface[MAX_CLAY_PARAMETER_LENGTH];
    char server_protocol[MAX_CLAY_PARAMETER_LENGTH];
    char server_port[MAX_CLAY_PARAMETER_LENGTH];
    char request_method[MAX_CLAY_PARAMETER_LENGTH];
    char script_name[MAX_CLAY_PARAMETER_LENGTH];
    char query_string[MAX_CLAY_PARAMETER_LENGTH];
    char remote_host[MAX_CLAY_PARAMETER_LENGTH];
    char remote_address[MAX_CLAY_PARAMETER_LENGTH];
    int incoming_socket;
} clay_variables;

typedef struct {
    char path[MAX_DYNAMIC_PATH_PREFIX];
    char endpoint[MAX_ENDPOINT];
    void* send_socket;
    void* receive_socket;
} clay_handler;

clay_variables build_clay_variables(struct spade_server* server,
				http_request* request, clay_handler* handler,
                int incoming_socket);

#endif // _CLAY_H_
