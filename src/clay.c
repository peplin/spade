#include "clay.h"
#include "server.h"

clay_variables build_clay_variables(spade_server* server, http_request* request,
        clay_handler* handler, int incoming_socket) {
    clay_variables variables;
    variables.incoming_socket = incoming_socket;

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
    if(request->remote_host[0] != '\0') {
        strcpy(variables.remote_host, request->remote_host);
    }
    strcpy(variables.remote_address, request->remote_address);

    return variables;
}
