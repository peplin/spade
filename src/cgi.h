#ifndef _CGI_H_
#define _CGI_H_

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "http.h"
#include "constants.h"

#define CGI_VERSION "1.1"

struct spade_server;

typedef struct {
    char handler[MAX_HANDLER_PATH_LENGTH];
    char path[MAX_DYNAMIC_PATH_PREFIX];
} cgi_handler;

void set_static_cgi_environment(struct spade_server* server);
void set_cgi_environment(struct spade_server* server, http_request* request, 
        cgi_handler* handler);

#endif // _CGI_H_
