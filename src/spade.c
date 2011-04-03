/* simple threaded http server */

#include "spade.h"

/* Proxy server struct is global so we can free it upon SIGINT. Nothing should
 * access this directly except free_server and main!
 */ 
spade_server global_server;

void free_server() {
    shutdown_server(&global_server);
    exit(0);
}

void print_help() {
    printf("spade - a concurrent web server\n");
    printf("Christopher Peplin, peplin@cmu.edu\n");
    printf("Options:\n");
    printf(" -p <port>   set the port for the server (default 8080)\n");
    printf(" -s <path>   set the path to static files to serve (default ./static)\n");
    printf(" -c <path>   set the path to the config file (default config/spade.cfg)\n");
    printf(" -h          display this dialogue\n");
}

int main(int argc, char *argv []) {
    if(argc > 5) {
        printf("Too many arguments\n");
        exit(1);
    }

    int c;
    unsigned int override_port = 0;
    char* configuration_path = DEFAULT_CONFIGURATION_FILE_PATH;
    while((c = getopt(argc, argv, "h:c:p:")) != -1) {
        switch(c) {
            case 'h':
                print_help();
                return 0;
            case 'p':
                override_port = atoi(optarg);
                break;
            case 'c':
                configuration_path = optarg;
                break;
            case '?':
                if (optopt == 'p') {
                    fprintf(stderr, "Option -%c requires an argument.\n", 
                            optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n",
                            optopt);
                }
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

    if(configure_server(&global_server, configuration_path, override_port)) {
        printf("Unable to configure server\n");
        exit(EXIT_FAILURE);
    }
    if(initialize_server(&global_server)) {
        printf("Unable to initialize server\n");
        exit(1);
    }
    signal(SIGINT, free_server);

    run_server(&global_server);
    free_server();

    return 0;
}
