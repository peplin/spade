#ifndef _TYPES_H_
#define _TYPES_H_

#include "constants.h"

typedef struct {
    char handler[MAX_HANDLER_PATH_LENGTH];
    char path[MAX_DYNAMIC_PATH_PREFIX];
} cgi_handler;

typedef struct {
    char handler[MAX_HANDLER_PATH_LENGTH];
    char path[MAX_DYNAMIC_PATH_PREFIX];
} dirt_handler;

/* Struct to hold server-wide settings and variables */
typedef struct spade_server {
    unsigned int port;
    char static_file_path[MAX_PATH_LENGTH];
    char dynamic_file_path[MAX_PATH_LENGTH];
    char hostname[MAX_HOSTNAME_LENGTH];
    int socket;
	int do_reverse_lookups;
    unsigned int cgi_handler_count;
    cgi_handler cgi_handlers[MAX_HANDLERS];
    unsigned int dirt_handler_count;
    dirt_handler dirt_handlers[MAX_HANDLERS];
    pthread_attr_t thread_attr; /* Attributes for receive threads */
} spade_server;

#endif // _TYPES_H_
