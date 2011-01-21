#include "server.h"

void return_client_error(int incoming_socket, char *cause, char* status_code,
        char *shortmsg, char *longmsg);
void return_response_headers(int incoming_socket, char* status_code,
        char* message, char* body, char* content_type, int length,
        int close_headers);
void serve_dynamic(spade_server* server, http_request* request, 
        int incoming_socket, dynamic_handler* handler);
void serve_static(spade_server* server, http_request* request,
        int incoming_socket);
void handle_get(spade_server* server, int incoming_socket,
        http_request* request);
void resolve_hostname(char* hostname, struct sockaddr_in* client_address);

/* Initialize socket for proxy server to listen on.
 *
 * Modifies server->socket.
 * Returns 0 if sucessful.
 */
int initialize_listen_socket(spade_server* server) {
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
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_WARN,
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

int initialize_server(spade_server* server) {
    pthread_attr_init(&server->thread_attr);
    pthread_attr_setstacksize(&server->thread_attr, 1024*1024);
    pthread_attr_setdetachstate(&server->thread_attr, PTHREAD_CREATE_DETACHED);

    if(initialize_listen_socket(server)) {
        return -1;
    }

    log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_INFO,
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
int read_line(rio_t* rio, char* buf, spade_server* server) {
    int bytes_read = rio_readlineb(rio, buf, MAXLINE);
    if(bytes_read > 0 && buf[bytes_read - 1] != '\n') {
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_WARN,
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
        spade_server* server) {
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
http_response read_http_response(rio_t* rio, spade_server* server) {
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
http_request read_http_request(rio_t* rio, spade_server* server) {
    char message_string[MAXLINE];
    http_request request;
    request.message.valid = 0;
    if(read_line(rio, message_string, server) > 0) {
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_DEBUG,
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
    strcpy(request.remote_address, inet_ntoa(args->client_address.sin_addr));
    if(args->server->do_reverse_lookups) {
        resolve_hostname(request.remote_host, &args->client_address);
    }

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
                        "Spade does not implement this method");
        }
    }
    log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_TRACE,
            "closing socket %d", args->incoming_socket);
    close(args->incoming_socket);
}

void resolve_hostname(char* hostname, struct sockaddr_in* client_address) {
    struct hostent *host_entry;
    if (NULL == (host_entry = gethostbyaddr(
            (char*) &client_address->sin_addr, sizeof(struct in_addr),
            AF_INET))) {
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_WARN,
                "Unable to resolve hostname for %s (errno %d)", 
                client_address->sin_addr, h_errno);
    } else {
        strcpy(hostname, host_entry->h_name);
    }
}

void handle_get(spade_server* server, int incoming_socket,
        http_request* request) {
    for (int i = 0; i < server->handler_count; i++) {
        if(!strncmp(server->handlers[i].path, request->uri.path, 
                    strlen(server->handlers[i].path))) {
            serve_dynamic(server, request, incoming_socket, 
                    &server->handlers[i]);
            return;
        }
    }
    serve_static(server, request, incoming_socket);
}

/* Helper function for new threads */
void* receive_helper(void* args) {
    signal(SIGPIPE, SIG_IGN);
    receive_args* arg_struct = (receive_args*) args;
    receive(arg_struct);
    free(arg_struct);
    return 0;
}

void run_server(spade_server* server) {
    pthread_t receive_thread;
    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_in client_address;    
    socklen_t sin_size = sizeof(struct sockaddr_in);

    while(1) {
        int message_socket = accept(server->socket, 
                (struct sockaddr *) &client_address, &sin_size);
        if(!check_error(message_socket, "accept")) {
            receive_args* receive_args = malloc(sizeof(receive_args));
            receive_args->server = server;
            receive_args->incoming_socket = message_socket;
            receive_args->client_address = client_address;
            pthread_create(&receive_thread, &server->thread_attr,
                    receive_helper, (void*) receive_args);
        }
    }
}

void shutdown_server(spade_server* server) {
}

void register_handler(spade_server* server, const char* path,
        const char* handler_path){
    dynamic_handler handler;
    strcpy(handler.path, path);
    strcpy(handler.handler, handler_path);

    char file_path[MAX_PATH_LENGTH];
    sprintf(file_path, "%s/%s", server->dynamic_file_path, handler.path);

    struct stat sbuf;
    if(stat(file_path, &sbuf) < 0) {
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_ERROR,
                "Couldn't find the handler file '%s' -- not adding handler",
                file_path);
    } else {
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_ERROR,
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
void serve_static(spade_server* server, http_request* request, 
        int incoming_socket) {

    char file_path[MAX_PATH_LENGTH];
    sprintf(file_path, "%s/%s", server->static_file_path, request->uri.path);

    struct stat sbuf;
    if(stat(file_path, &sbuf) < 0) {
        return_client_error(incoming_socket, request->uri.path, "404",
                "Not found", "Spade couldn't find this file");
        return;
    }

    if(S_ISDIR(sbuf.st_mode)) {
        strcat(file_path, "index.html");
        if(stat(file_path, &sbuf) < 0) {
            return_client_error(incoming_socket, request->uri.path, "404",
                    "Not found", "Spade couldn't find this file");
            return;
        }
    }

    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
        return_client_error(incoming_socket, request->uri.path, "403",
                "Forbidden", "Spade couldn't read the file");
        return;
    }

    int file_descriptor = open(file_path, O_RDONLY, 0);
    if(check_error(file_descriptor, "serve_static")) {
        return_client_error(incoming_socket, strerror(errno), "500",
                "Internal Server Error", "Spade crashed and burned.");
        return;
    }

    char content_type[MAXLINE];
    get_filetype(file_path, content_type);
    return_response_headers(incoming_socket, "200", "OK", NULL, content_type, 
            sbuf.st_size, 1);

    /* Send response body to client */
    char *srcp;
    srcp = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
    close(file_descriptor);
    if(srcp != (void*)-1) {
        csapp_rio_writen(incoming_socket, srcp, sbuf.st_size);
        munmap(srcp, sbuf.st_size);
    }
}

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
void serve_dynamic(spade_server* server, http_request* request,
        int incoming_socket, dynamic_handler* handler) {
    char file_path[MAX_PATH_LENGTH];
    sprintf(file_path, "%s/%s", server->dynamic_file_path, handler->handler);

    struct stat sbuf;
    stat(file_path, &sbuf);
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
        return_client_error(incoming_socket, request->uri.path, "403",
                "Forbidden", "Spade couldn't run the CGI program");
        return;
    }

    /* Return first part of HTTP response */
    return_response_headers(incoming_socket, "200", "OK", NULL, NULL, 0, 0);

    if(csapp_fork() == 0) { /* child */
        set_cgi_environment(server, request, handler);
        /* Redirect stdout to client */
        csapp_dup2(incoming_socket, STDOUT_FILENO);        
        char *emptylist[] = { NULL };
        csapp_execve(file_path, emptylist, environ);
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
    sprintf(body, "<html><title>Spade Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, status_code, short_message);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Spade Web server</em>\r\n", body);

    // TODO if the dynamic process doesn't close the headers, should we?
    return_response_headers(incoming_socket, status_code, short_message, body, 
            "text/html", 0, 1);
}

void return_response_headers(int incoming_socket, char* status_code,
        char* message, char* body, char* content_type, int length,
        int close_headers) {
    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.0 %s %s\r\n", status_code, message);
    csapp_rio_writen(incoming_socket, buf, strlen(buf));
    log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_DEBUG,
            "%s", buf);

    sprintf(buf, "Content-Type: %s\r\n", content_type);
    csapp_rio_writen(incoming_socket, buf, strlen(buf));

    if(body) {
        length = (int) strlen(body);
    }
    if (length != 0) {
        sprintf(buf, "Content-Length: %d\r\n", length);
        csapp_rio_writen(incoming_socket, buf, strlen(buf));
    }

    if(close_headers) {
        sprintf(buf, "\r\n");
        csapp_rio_writen(incoming_socket, buf, strlen(buf));

        if (body) {
            csapp_rio_writen(incoming_socket, body, strlen(body));
        }
    }
}
