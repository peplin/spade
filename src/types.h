#ifndef _TYPES_H_
#define _TYPES_H_

#include "constants.h"

typedef struct {
    char handler[MAX_HANDLER_PATH_LENGTH];
    char path[MAX_DYNAMIC_PATH_PREFIX];
} dynamic_handler;


/* Struct to hold server-wide settings and variables */
typedef struct spade_server {
    unsigned int port;
    char static_file_path[MAX_PATH_LENGTH];
    char dynamic_file_path[MAX_PATH_LENGTH];
    unsigned int handler_count;
    dynamic_handler handlers[MAX_HANDLERS];
    char hostname[MAX_HOSTNAME_LENGTH];
    int socket;
	int do_reverse_lookups;
    pthread_attr_t thread_attr; /* Attributes for receive threads */
} spade_server;

#endif // _TYPES_H_
