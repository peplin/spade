#include "cgi.h"

void set_static_cgi_environment(spade_server* server) {
    setenv("SERVER_SOFTWARE", SPADE_SERVER_DESCRIPTOR, 1);
    setenv("SERVER_NAME", server->hostname, 1);
    setenv("GATEWAY_INTERFACE", CGI_VERSION, 1);

    setenv("SERVER_PROTOCOL", "HTTP/1.0", 1);
    char stringified_port[MAX_PORT_LENGTH]; 
    sprintf(stringified_port, "%d", server->port);
    setenv("SERVER_PORT", stringified_port, 1);
}

void set_cgi_environment(spade_server* server, http_request* request,
        cgi_handler* handler) {
    setenv("REQUEST_METHOD", http_method_to_string(request->method), 1);

    char extra_path[MAX_PATH_LENGTH];
    strcpy(extra_path, request->uri.path + strlen(handler->handler));
    setenv("PATH_INFO", extra_path, 1);

    char translated_path[MAX_PATH_LENGTH];
    strcpy(translated_path, server->static_file_path);
    strcat(translated_path, extra_path);
    setenv("PATH_TRANSLATED", translated_path, 1);

    setenv("SCRIPT_NAME", handler->path, 1);
    setenv("QUERY_STRING", request->uri.query_string, 1);
    setenv("REMOTE_HOST", request->remote_host, 1);
    setenv("REMOTE_ADDR", request->remote_address, 1);
    // Spade only supports GET requests.
    // setenv("CONTENT_TYPE", TODO, 1);
    // setenv("CONTENT_LENGTH", TODO, 1);
}
