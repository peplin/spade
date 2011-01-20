#include "config.h"

void configure_port(dirt_server* server, unsigned int override_port,
        config_t* configuration);
void configure_static_file_path(dirt_server* server, config_t* configuration);
void configure_dynamic_file_path(dirt_server* server, config_t* configuration);
void configure_dynamic_handlers(dirt_server* server, config_t* configuration);

int configure_server(dirt_server* server, char* configuration_path,
        unsigned int override_port) {
    config_t configuration_struct, *configuration;
    configuration = &configuration_struct;
    config_init(configuration);
 
    if (!config_read_file(configuration, configuration_path)) {
        // TODO why are these undefined?
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_ERROR,
                "Configuration error: %s:%d - %s",
                configuration_path,
                config_error_line(configuration),
                config_error_text(configuration));
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
    if(config_lookup_string(configuration, "static.file_path",
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
    if(config_lookup_string(configuration, "dynamic.file_path",
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

void configure_dynamic_handlers(dirt_server* server, config_t* configuration) {
    config_setting_t* handler_settings = config_lookup(configuration,
            "dynamic.handlers");
    int handler_count = config_setting_length(handler_settings);

    for (int n = 0; n < handler_count; n++) {
        config_setting_t* handler_setting = config_setting_get_elem(
                handler_settings, n);
        const char* handler = NULL;
        config_setting_lookup_string(handler_setting, "handler", &handler);

        const char* url = NULL;
        config_setting_lookup_string(handler_setting, "url", &url);

        register_handler(server, url, handler);
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Registered handler '%s' for URL prefix '%s'", handler, url);
    }
    if (server->handler_count == 0) {
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "No dynamic URL handlers registered");
    } else {
        log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_INFO,
                "Registered a total of %d dynamic URL handlers",
                    server->handler_count);
    }
}
