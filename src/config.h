#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <libconfig.h>

#include "server.h"

#define DEFAULT_PORT 8080
#define DEFAULT_STATIC_FILE_PATH "static"
#define DEFAULT_CGI_FILE_PATH "static"
#define DEFAULT_DIRT_FILE_PATH "static"
#define DEFAULT_HOSTNAME "spade"

int configure_server(spade_server* server, char* configuration_path,
        unsigned int override_port);

#endif // _CONFIG_H_
