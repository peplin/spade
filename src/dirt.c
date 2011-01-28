#include "dirt.h"
#include "server.h"

dirt_variables build_dirt_variables(spade_server* server, http_request* request,
        dirt_handler* handler) {
    dirt_variables variables;

    strcpy(variables.server_software, SPADE_SERVER_DESCRIPTOR);
    strcpy(variables.server_name, server->hostname);
    strcpy(variables.gateway_interface, CGI_VERSION);

    strcpy(variables.server_protocol, "HTTP/1.0");
    char stringified_port[MAX_PORT_LENGTH]; 
    sprintf(stringified_port, "%d", server->port);
    strcpy(variables.server_port, stringified_port);
    strcpy(variables.request_method, http_method_to_string(request->method));

    strcpy(variables.script_name, handler->path);
    strcpy(variables.query_string, request->uri.query_string);
    strcpy(variables.remote_host, request->remote_host);
    strcpy(variables.remote_address, request->remote_address);

    return variables;
}
