#include "server.h"

// TODO rework zmq stuff to not use the logging from the example code
FILE* LOG_FILE = NULL;

void return_client_error(int incoming_socket, char *cause, char* status_code,
        char *shortmsg, char *longmsg);
int return_response_headers(int incoming_socket, char* status_code,
        char* message, char* body, char* content_type, int length,
        int close_headers);
void serve_cgi(spade_server* server, http_request* request,
        int incoming_socket, cgi_handler* handler);
void serve_dirt(spade_server* server, http_request* request,
        int incoming_socket, dirt_handler* handler);
void serve_clay(spade_server* server, http_request* request,
        int incoming_socket, clay_handler* handler);
void serve_static(spade_server* server, http_request* request,
        int incoming_socket);
int handle_get(spade_server* server, int incoming_socket,
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
    struct addrinfo* serv = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
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

    set_static_cgi_environment(server);

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
        http_header header;
        while(bytes_read > 0 && header_string[0] != '\n'
                && header_string[0] != '\r') {
            parse_http_header(&header, header_string);
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
    http_request request;
    request.message.valid = 0;
    char message_string[MAXLINE];
    if(read_line(rio, message_string, server) > 0) {
        char stripped_message_string[MAXLINE];
        strcpy(stripped_message_string, message_string);
        strstr(stripped_message_string, "\r\n")[0] = '\0';
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_DEBUG,
                "%s", stripped_message_string);
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
    request.remote_host[0] = '\0';
    request.remote_address[0] = '\0';
    if(args->server->do_reverse_lookups) {
        resolve_hostname(request.remote_host, &args->client_address);
    }

    int close_socket = 1;
    if(request.message.valid) {
        switch(request.method) {
            case HTTP_METHOD_GET:
                close_socket = handle_get(args->server, args->incoming_socket,
                        &request);
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
    if(close_socket == 1) {
        close(args->incoming_socket);
    }
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

int handle_get(spade_server* server, int incoming_socket,
        http_request* request) {
    for (int i = 0; i < server->cgi_handler_count; i++) {
        if(!strcmp(server->cgi_handlers[i].path, request->uri.path)) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_DEBUG,
                    "Serving request for path '%s' with CGI handler %s'",
                    request->uri.path, server->cgi_handlers[i].handler);
            serve_cgi(server, request, incoming_socket,
                    &server->cgi_handlers[i]);
            return 1;
        }
    }

    for (int i = 0; i < server->dirt_handler_count; i++) {
        if(!strcmp(server->dirt_handlers[i].path, request->uri.path)) {
            serve_dirt(server, request, incoming_socket,
                    &server->dirt_handlers[i]);
            return 1;
        }
    }

    for (int i = 0; i < server->clay_handler_count; i++) {
        if(!strcmp(server->clay_handlers[i].path, request->uri.path)) {
            serve_clay(server, request, incoming_socket,
                    &server->clay_handlers[i]);
            return 0;
        }
    }

    serve_static(server, request, incoming_socket);
    return 1;
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
            receive_args* args = malloc(sizeof(receive_args));
            args->server = server;
            args->incoming_socket = message_socket;
            args->client_address = client_address;
            pthread_create(&receive_thread, &server->thread_attr,
                    receive_helper, (void*) args);
        }
    }
}

void shutdown_server(spade_server* server) {
}

void* clay_receive_helper(void* socket) {
    signal(SIGPIPE, SIG_IGN);

    while(1) {
        zmq_msg_t msg;
        zmq_msg_init (&msg);
        clay_response response;
        zmq_recv(socket, &msg, 0);
        memcpy(&response, zmq_msg_data(&msg), zmq_msg_size(&msg));
        rio_writen(response.incoming_socket, response.response,
                response.response_length);
        close(response.incoming_socket);
        zmq_msg_close(&msg);
    }

    return 0;
}

int register_clay_handler(spade_server* server, const char* path,
        const char* endpoint){
    clay_handler handler;
    strcpy(handler.path, path);
    strcpy(handler.endpoint, endpoint);

    if(server->zmq_context == NULL) {
        server->zmq_context = zmq_init(ZMQ_THREAD_POOL_SIZE);
    }

    if(NULL == (handler.socket =
                zmq_socket(server->zmq_context, ZMQ_PAIR))) {
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_WARN,
                "Failed to create socket for context %s: %s",
                server->zmq_context, clean_errno());
        return -1;
    }

    log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_INFO,
            "Binding handler PAIR socket %s with identity: %s",
            handler.socket, handler.endpoint);

    int rc = zmq_bind(handler.socket, handler.endpoint);
    while(rc != 0) {
        sleep(1);
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_WARN,
                "Failed to bind send socket trying again for %s: %s",
                handler.endpoint, clean_errno());
        rc = zmq_bind(handler.socket, handler.endpoint);
    }

    pthread_create(&handler.receive_thread, &server->thread_attr,
            clay_receive_helper, handler.socket);

    server->clay_handlers[server->clay_handler_count] = handler;
    server->clay_handler_count++;
    return 0;
}

int register_dirt_handler(spade_server* server, const char* path,
        const char* function, const char* library){
    dirt_handler handler;
    strcpy(handler.path, path);

    char file_path[MAX_PATH_LENGTH];
    sprintf(file_path, "%s/%s", server->dirt_file_path, library);

    struct stat sbuf;
    if(stat(file_path, &sbuf) < 0) {
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_ERROR,
                "Couldn't find the shared library '%s' -- not adding handler",
                file_path);
        return -1;
    } else {
        void* library_handle = dlopen(file_path, RTLD_NOW);
        if (!library_handle) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR,
                    "Couldn't load the shared library '%s' -- not adding handler: %s",
                    file_path, dlerror());
            return -1;
        }
        handler.handler = dlsym(library_handle, function);
        char* error;
        if((error = dlerror()) != NULL) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR,
                    "Couldn't find the function '%s' in the shared library '%s' -- not adding handler: %s",
                    function, file_path, error);
            return -1;
        } else {
            server->dirt_handlers[server->dirt_handler_count] = handler;
            server->dirt_handler_count++;
        }
    }
    return 0;
}

int register_cgi_handler(spade_server* server, const char* path,
        const char* handler_path){
    cgi_handler handler;
    strcpy(handler.path, path);
    sprintf(handler.handler, "%s/%s", server->cgi_file_path, handler_path);

    struct stat sbuf;
    if(stat(handler.handler, &sbuf) < 0) {
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_ERROR,
                "Couldn't find the handler file '%s' -- not adding handler",
                handler.handler);
        return -1;
    } else {
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR,
                    "Couldn't run the handler file '%s' -- not adding handler",
                    handler.handler);
            return -1;
        } else {
            server->cgi_handlers[server->cgi_handler_count] = handler;
            server->cgi_handler_count++;
        }
    }
    return 0;
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
    if(-1 != return_response_headers(incoming_socket, "200", "OK", NULL,
                content_type, sbuf.st_size, 1)) {
        char *srcp;
        srcp = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, file_descriptor,
                0);
        close(file_descriptor);
        if(srcp != (void*)-1) {
            rio_writen(incoming_socket, srcp, sbuf.st_size);
            munmap(srcp, sbuf.st_size);
        }
    }
}

void serve_dirt(spade_server* server, http_request* request,
        int incoming_socket, dirt_handler* handler) {
    log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_DEBUG,
            "Handling request with a Dirt handler");
    if(-1 != return_response_headers(incoming_socket, "200", "OK", NULL, NULL,
                0, 0)) {
        (*handler->handler)(incoming_socket,
                build_dirt_variables(server, request, handler));
    }
}

void free_data(void* data, void* hint) {
    if(hint) {
        free(hint);
    }
}

void serve_clay(spade_server* server, http_request* request,
        int incoming_socket, clay_handler* handler) {
    log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_DEBUG,
            "Handling request with a Clay handler");
    int rc = 0;
    zmq_msg_t msg;
    if(-1 != return_response_headers(incoming_socket, "200", "OK", NULL, NULL,
                0, 0)) {
        clay_variables variables = build_clay_variables(server, request,
                handler, incoming_socket);

        if(0 != (rc = zmq_msg_init(&msg))) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR,
                    "Failed to initialize 0mq message to send.");
        }

        void* data = malloc(sizeof(variables));
        // TODO check that this is allocated
        memcpy(data, &variables, sizeof(variables));
        if(0 != (rc = zmq_msg_init_data(&msg, data, sizeof(variables),
                        free_data, data))) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR, "Failed to init 0mq message data.");
        }

        if(0 != (rc = zmq_send(handler->socket, &msg, 0))) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR,
                    "Failed to deliver 0mq message to handler.");
        }
    }
}

/*
 * serve_cgi - run a CGI program on behalf of the client
 */
void serve_cgi(spade_server* server, http_request* request,
        int incoming_socket, cgi_handler* handler) {
    struct stat sbuf;
    stat(handler->handler, &sbuf);
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
        return_client_error(incoming_socket, request->uri.path, "403",
                "Forbidden", "Spade couldn't run the CGI program");
        return;
    }

    if(-1 != return_response_headers(incoming_socket, "200", "OK", NULL, NULL,
                0, 0)) {
        if(fork() == 0) { /* child */
            set_cgi_environment(server, request, handler);
            /* Redirect stdout to client */
            dup2(incoming_socket, STDOUT_FILENO);
            char *emptylist[] = { NULL };
            execve(handler->handler, emptylist, environ);
        }
        wait(NULL); /* Parent waits for and reaps child */
    }
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

int return_response_headers(int incoming_socket, char* status_code,
        char* message, char* body, char* content_type, int length,
        int close_headers) {
    char buf[MAXLINE];

    sprintf(buf, "HTTP/1.0 %s %s\r\n", status_code, message);
    if(rio_writen(incoming_socket, buf, strlen(buf)) == -1) {
        log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_ERROR,
                "Couldn't write to socket: %s", strerror(errno));
        return -1;
    }
    strstr(buf, "\r\n")[0] = '\0';
    log4c_category_log(log4c_category_get("spade"), LOG4C_PRIORITY_DEBUG,
            "%s", buf);

    if(body) {
        length = (int) strlen(body);

        sprintf(buf, "Content-Type: %s\r\n", content_type);
        if(rio_writen(incoming_socket, buf, strlen(buf)) == -1) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR,
                    "Couldn't write to socket: %s", strerror(errno));
            return -1;
        }
    }
    if (length != 0) {
        sprintf(buf, "Content-Length: %d\r\n", length);
        if(rio_writen(incoming_socket, buf, strlen(buf)) == -1) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR, "Couldn't write to socket: %s",
                    strerror(errno));
            return -1;
        }
    }

    if(close_headers) {
        sprintf(buf, "\r\n");
        if(rio_writen(incoming_socket, buf, strlen(buf)) == -1) {
            log4c_category_log(log4c_category_get("spade"),
                    LOG4C_PRIORITY_ERROR, "Couldn't write to socket: %s",
                    strerror(errno));
            return -1;
        }

        if (body) {
            if(rio_writen(incoming_socket, body, strlen(body)) == -1) {
                log4c_category_log(log4c_category_get("spade"),
                        LOG4C_PRIORITY_ERROR, "Couldn't write to socket: %s",
                        strerror(errno));
                return -1;
            }
        }
    }
    return 0;
}
