#include "server.h"

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
        if(server->verbosity) {
            atomic_printf(&server->stdout_mutex,
                    "ERROR: getaddrinfo failed with error %d: %s\n", 
                    result, gai_strerror(result));
        }
        return result;
    }

    server->socket = 
        socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol);
    if(check_error(server->socket, "socket", &server->stdout_mutex,
                server->verbosity)) {
        return server->socket;
    }

    result = setsockopt(server->socket, SOL_SOCKET,
                            SO_REUSEADDR, &on, sizeof(on));
    if(check_error(result, "setsockopt", &server->stdout_mutex,
                server->verbosity)) {
        freeaddrinfo(serv);
        return result;
    }

    result = bind(server->socket, serv->ai_addr, serv->ai_addrlen);
    if(check_error(result, "bind", &server->stdout_mutex, server->verbosity)) {
        freeaddrinfo(serv);
        return result;
    }
    freeaddrinfo(serv);

    result = listen(server->socket, MAX_CONNECTION_QUEUE);
    if(check_error(result, "listen", &server->stdout_mutex, server->verbosity)) {
        return result;
    }
    
    return 0;
}

/* Initialize dirt_server struct server with values specified.
 *
 * Modifies *server.
 * Returns 0 if successful.
 */
int initialize_server(dirt_server* server, unsigned int port, int echo,
        int verbosity) {
    server->port = port;
    server->echo = echo;
    server->verbosity = verbosity;

    pthread_attr_init(&server->thread_attr);
    pthread_attr_setstacksize(&server->thread_attr, 1024*1024);
    pthread_attr_setdetachstate(&server->thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_mutex_init(&server->stdout_mutex, NULL);

    if(initialize_listen_socket(server)) {
        return -1;
    }

    if(verbosity > 1) {
        atomic_printf(&server->stdout_mutex, "Starting server on port %d\n", 
                server->port);
    }
    if(verbosity > 1 && server->echo) {
        atomic_printf(&server->stdout_mutex, "Server is in echo mode\n");
    }
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
    if(server->verbosity && bytes_read > 0 && buf[bytes_read - 1] != '\n') {
        atomic_printf(&server->stdout_mutex,
                "HTTP request line:\n%s with length %d longer than MAXLINE %d\n", 
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
        if(server->verbosity > 2) {
            atomic_printf(&server->stdout_mutex, "\n%s\n", message_string);
        }
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
    // TODO http_response response, *response_pointer;
    int destination_socket = 0;
    if(request.message.valid) {
        if(args->server->echo) {
            echo_request(&request, args->incoming_socket, args->server);
        } else {
            // TODO generate response
        }
    }
    if(args->server->verbosity > 2) {
        atomic_printf(&args->server->stdout_mutex, "\n_closing socket %d\n",
                args->incoming_socket);
    }
    if(destination_socket > 0) {
        close(destination_socket);
    }
    close(args->incoming_socket);
}

/* Helper function for new threads */
void* receive_helper(void* args) {
    signal(SIGPIPE, SIG_IGN); 
    receive_args* arg_struct = (receive_args*) args;
    receive(arg_struct);
    free(arg_struct);
    return 0;
}

/* Echo the processes request back to the client at socket. Used for testing. */
void echo_request(http_request* request, int socket, dirt_server* server) {
    char buffer[MAXBUF];
    http_request_to_string(request, buffer);
    if(server->verbosity > 1) {
        atomic_printf(&server->stdout_mutex, "Echoing request %s\n", buffer);
    }
    send(socket, buffer, strlen(buffer), 0);
}

/* Main thread for proxy server. Listens on the server socket and spawns
 * threads to handle new requests. Should never return.
 *
 * Requires server to be initialized with initialize_server.
 */
void run_server(dirt_server* server) {
    pthread_t receive_thread;
    signal(SIGPIPE, SIG_IGN); 

    while(1) {
        int message_socket = accept(server->socket, NULL, NULL);
        if(!check_error(message_socket, "accept", &server->stdout_mutex,
                    server->verbosity)) {
            receive_args* receive_args = malloc(sizeof(receive_args));
            receive_args->server = server;
            receive_args->incoming_socket = message_socket;
            pthread_create(&receive_thread, &server->thread_attr, receive_helper, 
                    (void*) receive_args);
        } 
    }
}

/*
 * serve_static - copy a file back to the client
 */
void serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);       
    sprintf(buf, "HTTP/1.0 200 OK\r\n");    
    sprintf(buf, "%s_server: Tiny Web Server\r\n", buf);
    sprintf(buf, "%s_content-length: %d\r\n", buf, filesize);
    sprintf(buf, "%s_content-type: %s\r\n\r\n", buf, filetype);
    csapp_rio_writen(fd, buf, strlen(buf));       

    /* Send response body to client */
    srcfd = csapp_open(filename, O_RDONLY, 0);    
    srcp = csapp_mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    csapp_close(srcfd);                           
    csapp_rio_writen(fd, srcp, filesize);         
    csapp_munmap(srcp, filesize);                 
}

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
void serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    csapp_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    csapp_rio_writen(fd, buf, strlen(buf));

    if (csapp_fork() == 0) { /* child */ 
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1); 
        csapp_dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ 
        csapp_execve(filename, emptylist, environ); /* Run CGI program */ 
    }
    csapp_wait(NULL); /* Parent waits for and reaps child */
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
         char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    csapp_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    csapp_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    csapp_rio_writen(fd, buf, strlen(buf));
    csapp_rio_writen(fd, body, strlen(body));
}
