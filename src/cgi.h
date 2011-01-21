#ifndef _CGI_H_
#define _CGI_H_

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "http.h"
#include "constants.h"
#include "types.h"
#include "server.h"

#define CGI_VERSION "1.1"

void set_cgi_environment(spade_server* server, http_request* request, 
        dynamic_handler* handler);

#endif // _CGI_H_
