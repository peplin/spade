/* simple libev-based http server */

#include "dirt.h"

static log4c_category_t* logger = NULL;

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd) {
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    csapp_rio_readinitb(&rio, fd);
    csapp_rio_readlineb(&rio, buf, MAXLINE);                   
    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET")) {                     
       clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
        return;
    }                                                    
    read_requesthdrs(&rio);                             

    /* Parse URI from GET request */
    http_uri parsed_uri = parse_http_uri(uri);
    if (stat(parsed_uri.path, &sbuf) < 0) {                     
        clienterror(fd, parsed_uri.path, "404", "Not found",
                "Tiny couldn't find this file");
        return;
    }                                                    

    if (parsed_uri.is_dynamic) { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { 
            clienterror(fd, parsed_uri.path, "403", "Forbidden",
                "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, parsed_uri.path, parsed_uri.query_string);            
    } else { /* Serve static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { 
            clienterror(fd, parsed_uri.path, "403", "Forbidden",
                "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, parsed_uri.path, sbuf.st_size);        
    }
}

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    csapp_rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {          
        csapp_rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

/* Proxy server struct is global so we can free it upon SIGINT. Nothing should
 * access this directly except free_server and main!
 */ 
dirt_server global_server;

void free_server() {
    // TODO shutdown server?
    exit(0);
}

void printHelp() {
    printf("15-213 proxy\n");
    printf("Christopher Peplin, cpeplin@andrew.cmu.edu\n");
    printf("Options\n");
    printf(" -v          set verbosity level\n");
    printf(" -p <port>   set the port for the server (default 8080)\n");
    printf(" -g          disable caching, proxy acts as a gateway only\n");
    printf(" -e          for testing, echo the request back to the client\n");
    printf(" -h          display this dialogue\n");

}

int main(int argc, char *argv []) {
    if(argc > 7) {
        printf("Too many arguments\n");
        exit(1);
    }

    int c;
    int echo = 0;
    int verbosity = 0;
    unsigned int port = DEFAULT_PORT;
    while((c = getopt(argc, argv, "hgv:ep:")) != -1) {
        switch(c) {
            case 'h':
                printHelp();
                return 0;
            case 'v':
                verbosity = atoi(optarg);
                break;
            case 'e':
                echo = 1;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case '?':
                if (optopt == 'p')
                    fprintf(stderr, "Option -%c requires an argument.\n", 
                            optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n",
                            optopt);
                printHelp();
                return 1;
            default:
                printHelp();
                abort();
        }
    }

    if(initialize_server(&global_server, port, echo, verbosity)) {
        printf("Unable to initialize server\n");
        exit(1);
    }
    signal(SIGINT, free_server);
    run_server(&global_server);

    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;

    if(log4c_init()) {
        printf("log4c init failed");
        exit(1);
    }
    logger = log4c_category_get("dirt");

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);
    log4c_category_log(logger, LOG4C_PRIORITY_INFO,
            "Starting dirt web server on port %d", port);

    listenfd = csapp_open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = csapp_accept(listenfd, (SA *)&clientaddr,
                        (socklen_t*)&clientlen);
        doit(connfd);
        csapp_close(connfd);
    }

    free_server();
    return 0;
}
