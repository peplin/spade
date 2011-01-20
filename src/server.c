#include "server.h"

void return_client_error(int incoming_socket, char *cause, char* status_code,
        char *shortmsg, char *longmsg);
void return_response_headers(int incoming_socket, char* status_code,
        char* message, char* body, char* content_type, int length);
void serve_dynamic(int incoming_socket, char *filename, char *cgiargs);
void serve_static(dirt_server* server, http_request* request,
        int incoming_socket, char *filename, int filesize);
void handle_get(dirt_server* server, int incoming_socket,
        http_request* request);

/* Initialize socket for proxy server to listen on.
 *
 * Modifies server->socket.
 * Returns 0 if sucessful.
 */
int initialize_listen_socket(dirt_server* server) {
    int result;
    int on = 1;
    char port[MAX_PORT_LENGTH];
    sprintf(port, "%d", server->port);
    struct addrinfo* serv = NULL, hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, port, &hints, &serv);
    if(result < 0) {
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_DEBUG,
                "getaddrinfo failed with error %d: %s",
                result, gai_strerror(result));
        return result;
    }

    server->socket =
        socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol);
    if(check_error(server->socket, "socket")) {
        return server->socket;
    }

    result = setsockopt(server->socket, SOL_SOCKET,
                            SO_REUSEADDR, &on, sizeof(on));
    if(check_error(result, "setsockopt")) {
        freeaddrinfo(serv);
        return result;
    }

    result = bind(server->socket, serv->ai_addr, serv->ai_addrlen);
    if(check_error(result, "bind")) {
        freeaddrinfo(serv);
        return result;
    }
    freeaddrinfo(serv);

    result = listen(server->socket, MAX_CONNECTION_QUEUE);
    if(check_error(result, "listen")) {
        return result;
    }

    return 0;
}

int initialize_server(dirt_server* server) {
    pthread_attr_init(&server->thread_attr);
    pthread_attr_setstacksize(&server->thread_attr, 1024*1024);
    pthread_attr_setdetachstate(&server->thread_attr, PTHREAD_CREATE_DETACHED);

    if(initialize_listen_socket(server)) {
        return -1;
    }

    log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
            "Starting server on port %d, serving files frome %s",
            server->port, server->static_file_path);
    return 0;
}

/* Read a line from the rio buffer. Just a wrapper to read the line, print
 * a log statement and check that it ends in a newline if necessary.
 *
 * Modifies rio, buf.
 * Returns the number of bytes read, or a negative number if an error.
 */
int read_line(rio_t* rio, char* buf, dirt_server* server) {
    int bytes_read = rio_readlineb(rio, buf, MAXLINE);
    if(bytes_read > 0 && buf[bytes_read - 1] != '\n') {
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_WARN,
                "HTTP request line:\n%s with length %d longer than MAXLINE %d",
                buf, bytes_read, MAXLINE);
    }
    return bytes_read;
}

/* Read HTTP headers from the rio buffer until CRLF is found or there are no
 * more bytes read.
 *
 * Only stores valid headers.
 *
 * Modifies rio, message.
 */
void read_http_headers(rio_t* rio, http_message* message,
        dirt_server* server) {
    if(rio->rio_cnt) {
        char header_string[MAXLINE];
        int bytes_read = read_line(rio, header_string, server);
        while(bytes_read > 0 && header_string[0] != '\n'
                && header_string[0] != '\r') {
            http_header header = parse_http_header(header_string);
            if(header.valid) {
                message->headers[message->header_count++] = header;
                bytes_read = read_line(rio, header_string, server);
            }
        }
    }
}

/* Read the HTTP response status line from the rio buffer.
 * Returns the parsed HTTP response, which may or may not be have
 * response.valid.
 *
 * Modifies rio.
 */
http_response read_http_response(rio_t* rio, dirt_server* server) {
    char message_string[MAXLINE];
    http_response response;
    response.message.valid = 0;
    if(read_line(rio, message_string, server) > 0) {
        response = parse_http_response(message_string);
        read_http_headers(rio, &response.message, server);
    }
    return response;
}

/* Read the HTTP request from the rio buffer, including any headers.
 *
 * Does not read content following the headers.
 *
 * Modifies rio.
 * Returns the parsed request, which may or may not have request.valid.
 */
http_request read_http_request(rio_t* rio, dirt_server* server) {
    char message_string[MAXLINE];
    http_request request;
    request.message.valid = 0;
    if(read_line(rio, message_string, server) > 0) {
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_DEBUG,
                "%s", message_string);
        request = parse_http_request(message_string);
        read_http_headers(rio, &request.message, server);
    }
    return request;
}

/* Receiving thread primary function. Receives client request and any content,
 * generates a response and returns it back to the client.
 *
 */
void receive(receive_args* args) {
    rio_t rio_client;
    rio_readinitb(&rio_client, args->incoming_socket);
    http_request request = read_http_request(&rio_client, args->server);
    if(request.message.valid) {
        switch(request.method) {
            case HTTP_METHOD_GET:
                handle_get(args->server, args->incoming_socket, &request);
                break;
            default:
                return_client_error(args->incoming_socket,
                        http_method_to_string(request.method),
                        "501",
                        "Not Implemented",
                        "Dirt does not implement this method");
        }
    }
    log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_TRACE,
            "closing socket %d", args->incoming_socket);
    close(args->incoming_socket);
}

void handle_get(dirt_server* server, int incoming_socket,
        http_request* request) {
    struct stat sbuf;
    char file_path[MAX_PATH_LENGTH];

    for (int i = 0; i < server->handler_count; i++) {
        sprintf(file_path, "%s/%s", server->dynamic_file_path,
                server->handlers[i].handler);
        stat(file_path, &sbuf);
        if(!strcmp(server->handlers[i].path, request->uri.path)) {
            if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
                return_client_error(incoming_socket, request->uri.path, "403",
                        "Forbidden", "Dirt couldn't run the CGI program");
                return;
            }
            serve_dynamic(incoming_socket, file_path,
                    request->uri.query_string);
            return;
        }
    }

    sprintf(file_path, "%s/%s", server->static_file_path, request->uri.path);
    if(stat(file_path, &sbuf) < 0) {
        return_client_error(incoming_socket, request->uri.path, "404",
                "Not found", "Dirt couldn't find this file");
        return;
    }

    if(S_ISDIR(sbuf.st_mode)) {
        strcat(file_path, "index.html");
        if(stat(file_path, &sbuf) < 0) {
            return_client_error(incoming_socket, request->uri.path, "404",
                    "Not found", "Dirt couldn't find this file");
            return;
        }
    }

    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
        return_client_error(incoming_socket, request->uri.path, "403",
                "Forbidden", "Dirt couldn't read the file");
        return;
    }
    serve_static(server, request, incoming_socket, file_path, sbuf.st_size);
}

/* Helper function for new threads */
void* receive_helper(void* args) {
    signal(SIGPIPE, SIG_IGN);
    receive_args* arg_struct = (receive_args*) args;
    receive(arg_struct);
    free(arg_struct);
    return 0;
}

void run_server(dirt_server* server) {
    pthread_t receive_thread;
    signal(SIGPIPE, SIG_IGN);

    while(1) {
        int message_socket = accept(server->socket, NULL, NULL);
        if(!check_error(message_socket, "accept")) {
            receive_args* receive_args = malloc(sizeof(receive_args));
            receive_args->server = server;
            receive_args->incoming_socket = message_socket;
            pthread_create(&receive_thread, &server->thread_attr,
                    receive_helper, (void*) receive_args);
        }
    }
}

void shutdown_server(dirt_server* server) {
}

void register_handler(dirt_server* server, const char* path,
        const char* handler_path){
    dynamic_handler handler;
    strcpy(handler.path, path);
    strcpy(handler.handler, handler_path);

    char file_path[MAX_PATH_LENGTH];
    sprintf(file_path, "%s/%s", server->dynamic_file_path, handler.path);

    struct stat sbuf;
    if(stat(file_path, &sbuf) < 0) {
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_ERROR,
                "Couldn't find the handler file '%s' -- not adding handler",
                file_path);
    } else {
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_ERROR,
                    "Couldn't run the handler file '%s' -- not adding handler",
                    file_path);
        }
        server->handlers[server->handler_count] = handler;
        server->handler_count++;
    }
}

/*
 * serve_static - copy a file back to the client
 */
void serve_static(dirt_server* server, http_request* request, 
        int incoming_socket, char *filename, int filesize) {
    int file_descriptor = open(filename, O_RDONLY, 0);
    if(check_error(file_descriptor, "serve_static")) {
        return_client_error(incoming_socket, strerror(errno), "500",
                "Internal Server Error", "Dirt crashed and burned.");
        return;
    }

    char filetype[MAXLINE];
    get_filetype(filename, filetype);
    return_response_headers(incoming_socket, "200", "OK", NULL, filetype, 
            filesize);

    /* Send response body to client */
    char *srcp;
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
    close(file_descriptor);
    if(srcp != (void*)-1) {
        csapp_rio_writen(incoming_socket, srcp, filesize);
        munmap(srcp, filesize);
    }
}

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
void serve_dynamic(int incoming_socket, char *filename, char *cgiargs) {
    char *emptylist[] = { NULL };

    // TODO the dynamic part should have control over the response code and
    // headers, no?
    /* Return first part of HTTP response */
    return_response_headers(incoming_socket, "200", "OK", NULL, NULL, 0);

    if(csapp_fork() == 0) { /* child */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);
        /* Redirect stdout to client */
        csapp_dup2(incoming_socket, STDOUT_FILENO);        
        csapp_execve(filename, emptylist, environ); /* Run CGI program */
    }
    csapp_wait(NULL); /* Parent waits for and reaps child */
}

/*
 * return_client_error - returns an error message to the client
 */
void return_client_error(int incoming_socket, char *cause, char *status_code,
        char *short_message, char *longmsg) {
    char body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Dirt Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, status_code, short_message);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Dirt Web server</em>\r\n", body);

    return_response_headers(incoming_socket, status_code, short_message, body, 
            NULL, 0);
}

void return_response_headers(int incoming_socket, char* status_code,
        char* message, char* body, char* content_type, int length) {
    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.0 %s %s\r\n", status_code, message);
    csapp_rio_writen(incoming_socket, buf, strlen(buf));

    if(content_type) {
        sprintf(buf, "Content-Type: %s\r\n\r\n", content_type);
    } else {
        sprintf(buf, "Content-Type: text/html\r\n");
    }
    csapp_rio_writen(incoming_socket, buf, strlen(buf));

    if (body) {
        sprintf(buf, "Content-Length: %d\r\n\r\n", (int)strlen(body));
    } else {
        sprintf(buf, "Content-Length: %d\r\n\r\n", length);
    }
    csapp_rio_writen(incoming_socket, buf, strlen(buf));

    if (body) {
        csapp_rio_writen(incoming_socket, body, strlen(body));
    }
}
