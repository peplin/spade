#include "config.h"

void configure_port(dirt_server* server, unsigned int override_port,
        config_t* configuration);
void configure_static_file_path(dirt_server* server, config_t* configuration);
void configure_dynamic_file_path(dirt_server* server, config_t* configuration);

int configure_server(dirt_server* server, char* configuration_path,
        unsigned int override_port) {
    config_t configuration_struct, *configuration;
    configuration = &configuration_struct;
    config_init(configuration);
 
    if (!config_read_file(configuration, configuration_path)) {
        //fprintf(stderr, "%s:%d - %s\n",
        //    config_error_file(configuration),
        //    config_error_line(configuration),
        //    config_error_text(configuration));
        config_destroy(configuration);
        return(EXIT_FAILURE);
    }

    configure_port(server, override_port, configuration);
    configure_static_file_path(server, configuration);
    configure_dynamic_file_path(server, configuration);

    config_destroy(configuration);
    return 0;
}

void configure_port(dirt_server* server, unsigned int override_port,
        config_t* configuration) {
    if (override_port) {
        server->port = override_port;
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Using override port %d", server->port);
    } else if(config_lookup_int(configuration, "port",
            (long int *)&server->port)) {
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Using port %d from configuration file", server->port);
    } else {
        server->port = DEFAULT_PORT;
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Using default port %d", server->port);
    }
}

void configure_static_file_path(dirt_server* server, config_t* configuration) {
    const char* static_file_path = NULL;
    if(config_lookup_string(configuration, "static_file_path",
            &static_file_path)) {
        strcpy(server->static_file_path, static_file_path);
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Using static file path '%s' from configuration file",
                server->static_file_path);
    } else {
        strcpy(server->static_file_path, DEFAULT_STATIC_FILE_PATH);
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Using default static file path '%s'", server->static_file_path);
    }
}

void configure_dynamic_file_path(dirt_server* server, config_t* configuration) {
    const char* dynamic_file_path = NULL;
    if(config_lookup_string(configuration, "dynamic_file_path",
            &dynamic_file_path)) {
        strcpy(server->dynamic_file_path, dynamic_file_path);
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Using static file path '%s' from configuration file",
                server->dynamic_file_path);
    } else {
        strcpy(server->dynamic_file_path, DEFAULT_STATIC_FILE_PATH);
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Using default static file path '%s'", server->static_file_path);
    }
}
