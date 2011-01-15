#include "http.h"

void free_http_response(http_response* response) {
    if(response->message.length && response->data) {
        free(response->data);
    }
}

int equal_http_headers(http_header* first, int first_count,
        http_header* second, int second_count) {
    int i, j;
    for(i = 0; i < first_count; i++) {
        for(j = 0; j < second_count; j++) {
            if(!strncmp(first[i].key, second[j].key, MAX_HEADER_KEY_LENGTH)) {
                if(strncmp(first[i].value, second[j].value,
                            MAX_HEADER_VALUE_LENGTH)) {
                    return 0;
                }
            }
        }
    }

    for(i = 0; i < second_count; i++) {
        for(j = 0; j < first_count; j++) {
            if(!strncmp(second[i].key, first[j].key, MAX_HEADER_KEY_LENGTH)) {
                if(strncmp(second[i].value, first[j].value,
                            MAX_HEADER_VALUE_LENGTH)) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

int find_matching_http_header(char* key, http_header* match, http_header* second, 
        int second_count) {
    int i;
    if(!strncmp(match->key, key, MAX_HEADER_KEY_LENGTH)) {
        for(i = 0; i < second_count; i++) {
            if(!strncmp(second[i].key, key, MAX_HEADER_KEY_LENGTH)) {
                if(strncmp(match->value, second[i].value, 
                        MAX_HEADER_VALUE_LENGTH)) {
                    return 0;
                } else {
                    break;
                }
            }
        }
    }
    return 1;
}

int equal_cached_http_headers(http_header* first, int first_count,
        http_header* second, int second_count) {
    int i;
    for(i = 0; i < first_count; i++) {
        if(!find_matching_http_header("Accept", &first[i], second, second_count)) {
            return 0;
        }
        if(!find_matching_http_header("Accept-Language", &first[i], second,
                    second_count)) {
            return 0;
        }
        if(!find_matching_http_header("Accept-Encoding", &first[i], second,
                    second_count)) {
            return 0;
        }
    }
    return 1;
}

int equal_http_uri(http_uri* first, http_uri* second) {
    return !strncmp(first->host, second->host, MAX_HOSTNAME_LENGTH)
        && first->port == second->port
        && !strncmp(first->path, second->path, MAX_PATH_LENGTH)
        && first->valid == second->valid;
}

int equal_http_message(http_message* first, http_message* second) {
    return first->version == second->version
            && first->header_count == second->header_count
            && equal_http_headers(first->headers, first->header_count,
                    second->headers, second->header_count)
            && first->length == second->length
            && first->valid == second->valid;
}

int equal_http_request(http_request* first, http_request* second) {
    return first->method == second->method
            && equal_http_uri(&first->uri, &second->uri)
            && first->has_host_header && second->has_host_header
            && equal_http_message(&first->message, &second->message);
}

int equal_cached_http_request(http_request* first, http_request* second) {
    return first->method == second->method
            && equal_http_uri(&first->uri, &second->uri)
            && equal_cached_http_headers(first->message.headers, 
                    first->message.header_count,
                    second->message.headers, second->message.header_count);
}

http_version string_to_http_version(char* version) {
    http_version result;
    if(!strncmp(version, HTTP_VERSION_1_0_STRING, MAX_VERSION_LENGTH)) {
        result = HTTP_VERSION_1_0;
    } else if(!strncmp(version, HTTP_VERSION_1_1_STRING, MAX_VERSION_LENGTH)) {
        result = HTTP_VERSION_1_1;
    } else {
        result = HTTP_VERSION_NONE;
    }
    return result;
}

char* http_version_to_string(http_version version) {
    if(version == HTTP_VERSION_1_0) {
        return HTTP_VERSION_1_0_STRING;
    } else if(version == HTTP_VERSION_1_1) {
        return HTTP_VERSION_1_1_STRING;
    } else {
        return HTTP_VERSION_NONE_STRING;
    }
}

http_method string_to_http_method(char* method) {
    http_method result;
    if(!strcmp(method, HTTP_METHOD_GET_STRING)) {
        result = HTTP_METHOD_GET;
    } else if(!strcmp(method, HTTP_METHOD_HEAD_STRING)) {
        result = HTTP_METHOD_HEAD;
    } else if(!strcmp(method, HTTP_METHOD_OPTIONS_STRING)) {
        result = HTTP_METHOD_OPTIONS;
    } else if(!strcmp(method, HTTP_METHOD_POST_STRING)) {
        result = HTTP_METHOD_POST;
    } else if(!strcmp(method, HTTP_METHOD_PUT_STRING)) {
        result = HTTP_METHOD_PUT;
    } else if(!strcmp(method, HTTP_METHOD_DELETE_STRING)) {
        result = HTTP_METHOD_DELETE;
    } else {
        result = HTTP_METHOD_NONE;
    }
    return result;
}

char* http_method_to_string(http_method method) {
    if(method == HTTP_METHOD_GET) {
        return HTTP_METHOD_GET_STRING;
    } else if(method == HTTP_METHOD_HEAD) {
        return HTTP_METHOD_HEAD_STRING;
    } else if(method == HTTP_METHOD_OPTIONS) {
        return HTTP_METHOD_OPTIONS_STRING;
    } else if(method == HTTP_METHOD_POST) {
        return HTTP_METHOD_POST_STRING;
    } else if(method == HTTP_METHOD_PUT) {
        return HTTP_METHOD_PUT_STRING;
    } else if(method == HTTP_METHOD_DELETE) {
        return HTTP_METHOD_DELETE_STRING;
    } else {
        return HTTP_METHOD_NONE_STRING;
    }
}

void http_headers_to_string(http_message* message, char* buf) {
    int i;
    int header_buffer_size = MAX_HEADER_KEY_LENGTH
            + MAX_HEADER_VALUE_LENGTH + 3;
    char header[header_buffer_size];
    for(i = 0; i < message->header_count; i++) {
        if(strncmp(message->headers[i].key, "Keep-Alive",
                    MAX_HEADER_KEY_LENGTH)) {
            snprintf(header, header_buffer_size, "%s: %s\r\n", 
                    message->headers[i].key, message->headers[i].value);
            buf = strcat(buf, header);
        }
    }
}

char* http_request_to_string(http_request* request, char* buf) {
    snprintf(buf, MAXBUF, "%s %s %s\r\n", 
            http_method_to_string(request->method), request->uri.path,
            HTTP_VERSION_1_0_STRING);
    http_headers_to_string(&request->message, buf);
    buf = strcat(buf, "\r\n");
    return buf;
}

char* http_response_to_string(http_response* response, char* buf) {
    snprintf(buf, MAXBUF, "%s %d %s\r\n", 
            http_version_to_string(response->message.version), response->status, 
            response->status_reason);
    http_headers_to_string(&response->message, buf);
    buf = strcat(buf, "\r\n");
    return buf;
}

http_header parse_http_header(char* header) {
    http_header parsed_header;
    strncpy(parsed_header.key, "", 1);
    strncpy(parsed_header.value, "", 1);
    parsed_header.valid = 1;
    sscanf(header, 
            "%"XSTR(MAX_HEADER_KEY_LENGTH)"[^:]: %"XSTR(MAX_HEADER_VALUE_LENGTH)"[^\r\n]", 
            parsed_header.key, parsed_header.value);
    if(!strncmp(parsed_header.key, "", 1)) {
        parsed_header.valid = 0;
    }
    return parsed_header;
}

http_uri parse_http_uri(char* uri) {
    http_uri parsed_uri;
    parsed_uri.valid = 0;
    parsed_uri.is_dynamic = 0;
    parsed_uri.query_string[0] = '\0';
    parsed_uri.port = DEFAULT_HTTP_PORT;
    parsed_uri.host[0] = '\0';
    char* port;
    char* protocol_index;
    if((protocol_index = strstr(uri, "://"))) {
        uri = protocol_index + 3;
    }

    /* Proxy expects http://hostname/... style requests */
    int tokens = sscanf(uri,
            "%"XSTR(MAX_HOSTNAME_LENGTH)"[^/]%"XSTR(MAX_PATH_LENGTH)"[^\r\n]", 
            parsed_uri.host,
            parsed_uri.path);
    if(tokens > 0) {
        if((port = strchr(parsed_uri.host, ':'))) {
            *port++ = '\0';
            parsed_uri.port = atoi(port);
        }
        parsed_uri.valid = 1;
    }
    if(tokens == 1 || parsed_uri.path[0] == '\0') {
        strncpy(parsed_uri.path, "/", 2);
    }

    char* query_string_index = strchr(parsed_uri.path, '?');
    if(query_string_index) {
        parsed_uri.is_dynamic = 1;
        strcpy(parsed_uri.query_string, query_string_index + 1);
        *query_string_index = '\0';
    }
    return parsed_uri;
}

http_request parse_http_request(char* request) {
    char uri[MAXLINE];
    char version[MAX_VERSION_LENGTH];
    char method[MAX_METHOD_LENGTH];
    version[0] = uri[0] = method[0] = '\0';
    http_request parsed_request;
    parsed_request.message.valid = 1;
    sscanf(request, 
            "%"XSTR(MAX_METHOD_LENGTH)"s %"XSTR(MAX_URI_LENGTH)"s %"XSTR(MAX_VERSION_LENGTH)"s", 
            method, uri, version);

    parsed_request.method = string_to_http_method(method);
    if(parsed_request.method == HTTP_METHOD_NONE) {
        parsed_request.message.valid = 0;
    }

    parsed_request.message.version = string_to_http_version(version);
    if(parsed_request.message.version == HTTP_VERSION_NONE) {
        parsed_request.message.valid = 0;
    }

    if(uri[0]) {
        parsed_request.uri = parse_http_uri(uri);
    } else {
        parsed_request.message.valid = 0;
    }
    parsed_request.message.header_count = 0;
    return parsed_request;
}

http_response parse_http_response(char* response) {
    char version[MAX_VERSION_LENGTH];
    version[0] = '\0';
    http_response parsed_response;
    parsed_response.message.valid = 1;
    parsed_response.message.length = 0;
    parsed_response.message.header_count = 0;
    parsed_response.status = 0;
    parsed_response.from_cache = 0;
    parsed_response.data = NULL;
    sscanf(response,
            "%"XSTR(MAX_VERSION_LENGTH)"s %"XSTR(MAX_STATUS_LENGTH)"d %"XSTR(MAX_STATUS_MESSAGE_LENGTH)"[^\r\n]",
            version, &parsed_response.status, parsed_response.status_reason);

    parsed_response.message.version = string_to_http_version(version);
    if(parsed_response.message.version == HTTP_VERSION_NONE) {
        parsed_response.message.valid = 0;
    }

    if(parsed_response.status < 100 || parsed_response.status > 510) {
        parsed_response.message.valid = 0;
    }

    parsed_response.message.header_count = 0;
    return parsed_response;
}
