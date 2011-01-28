#ifndef _DIRT_H_
#define _DIRT_H_

#define _GNU_SOURCE

#include "http.h"
#include "constants.h"

#define MAX_DIRT_PARAMETER_LENGTH 255

struct spade_server;

typedef struct {
    char server_software[MAX_DIRT_PARAMETER_LENGTH];
    char server_name[MAX_DIRT_PARAMETER_LENGTH];
    char gateway_interface[MAX_DIRT_PARAMETER_LENGTH];
    char server_protocol[MAX_DIRT_PARAMETER_LENGTH];
    char server_port[MAX_DIRT_PARAMETER_LENGTH];
    char request_method[MAX_DIRT_PARAMETER_LENGTH];
    char script_name[MAX_DIRT_PARAMETER_LENGTH];
    char query_string[MAX_DIRT_PARAMETER_LENGTH];
    char remote_host[MAX_DIRT_PARAMETER_LENGTH];
    char remote_address[MAX_DIRT_PARAMETER_LENGTH];
} dirt_variables;

typedef struct {
    void (*handler)(int incoming_socket, dirt_variables environment);
    char path[MAX_DYNAMIC_PATH_PREFIX];
} dirt_handler;

dirt_variables build_dirt_variables(struct spade_server* server,
				http_request* request, dirt_handler* handler);

#endif // _DIRT_H_
