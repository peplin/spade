/* simple libev-based http server */

#include "dirt.h"

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
    shutdown_server(&global_server);
    exit(0);
}

void printHelp() {
    printf("dirt - a concurrent web server\n");
    printf("Christopher Peplin, peplin@cmu.edu\n");
    printf("Options:\n");
    printf(" -v          set verbosity level\n");
    printf(" -p <port>   set the port for the server (default 8080)\n");
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

    if(log4c_init()) {
        printf("log4c init failed");
        exit(1);
    }
    log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
            "Starting dirt web server on port %d", port);

    if(initialize_server(&global_server, port, echo, verbosity)) {
        printf("Unable to initialize server\n");
        exit(1);
    }
    signal(SIGINT, free_server);

    run_server(&global_server);
    free_server();

    return 0;
}
