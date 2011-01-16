/* simple libev-based http server */

#include "dirt.h"

/* Proxy server struct is global so we can free it upon SIGINT. Nothing should
 * access this directly except free_server and main!
 */ 
dirt_server global_server;

void free_server() {
    shutdown_server(&global_server);
    exit(0);
}

void print_help() {
    printf("dirt - a concurrent web server\n");
    printf("Christopher Peplin, peplin@cmu.edu\n");
    printf("Options:\n");
    printf(" -p <port>   set the port for the server (default 8080)\n");
    printf(" -h          display this dialogue\n");
}

int main(int argc, char *argv []) {
    if(argc > 7) {
        printf("Too many arguments\n");
        exit(1);
    }

    int c;
    unsigned int port = DEFAULT_PORT;
    while((c = getopt(argc, argv, "h:p:")) != -1) {
        switch(c) {
            case 'h':
                print_help();
                return 0;
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
                print_help();
                return 1;
            default:
                print_help();
                abort();
        }
    }

    if(log4c_init()) {
        printf("log4c init failed");
        exit(1);
    }

    if(initialize_server(&global_server, port)) {
        printf("Unable to initialize server\n");
        exit(1);
    }
    signal(SIGINT, free_server);

    run_server(&global_server);
    free_server();

    return 0;
}
