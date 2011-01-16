#ifndef _HTTP_H_
#define _HTTP_H_

#include <log4c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csapp.h"

/**
 * 15-845 Independent Project
 *
 * Christopher Peplin
 * cpeplin@andrew.cmu.edu
 *
 * http.h/.c, HTTP request and response parsing and utilities
 */

/* These macros are used to turn numeric #define'd values into strings,
 * specifically for use in sscanf.
 */
#define STRINGIZE(s) #s
#define XSTR(s) STRINGIZE(s)

#define DEFAULT_HTTP_PORT 80

/* Maximum lengths of various strings to use stack allocated buffers. */
#define MAX_HOSTNAME_LENGTH 1024
#define MAX_PATH_LENGTH 1024
#define MAX_QUERY_STRING_LENGTH 1024
#define MAX_HEADER_KEY_LENGTH 256
#define MAX_HEADER_VALUE_LENGTH 1024
#define MAX_PORT_LENGTH 8
#define MAX_VERSION_LENGTH 9
#define MAX_METHOD_LENGTH 8
#define MAX_URI_LENGTH 8192
#define MAX_STATUS_LENGTH 3
#define MAX_STATUS_MESSAGE_LENGTH 32

/* Maximum number of HTTP headers, used for stack allocated buffer. */
#define HTTP_HEADER_LIST_LENGTH 64

#define HTTP_VERSION_1_0_STRING "HTTP/1.0"
#define HTTP_VERSION_1_1_STRING "HTTP/1.1"
#define HTTP_VERSION_NONE_STRING "HTTP/NONE"

#define HTTP_METHOD_GET_STRING "GET"
#define HTTP_METHOD_HEAD_STRING "HEAD"
#define HTTP_METHOD_OPTIONS_STRING "OPTIONS"
#define HTTP_METHOD_POST_STRING "POST"
#define HTTP_METHOD_PUT_STRING "PUT"
#define HTTP_METHOD_DELETE_STRING "DELETE"
#define HTTP_METHOD_NONE_STRING ""

typedef enum {
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_NONE
} http_version;

typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_NONE
} http_method;

typedef struct {
    char key[MAX_HEADER_KEY_LENGTH];
    char value[MAX_HEADER_VALUE_LENGTH];
    int valid;
} http_header;

typedef struct {
    char host[MAX_HOSTNAME_LENGTH];
    int port;
    char path[MAX_PATH_LENGTH];
    int valid;
    int is_dynamic;
    char query_string[MAX_QUERY_STRING_LENGTH];
} http_uri;

typedef struct {
    http_version version;
    http_header headers[HTTP_HEADER_LIST_LENGTH];
    unsigned int header_count;
    int length;
    int valid;
} http_message;

typedef struct {
    http_method method;
    http_uri uri;
    int has_host_header;
    http_message message;
} http_request;

typedef struct {
    int status;
    int from_cache;
    char status_reason[MAX_STATUS_MESSAGE_LENGTH];
    http_message message;
    void* data; /* Requests don't have data since we only accept GET for now */
} http_response;

/* "Public" utility methods */

/* Free an HTTP response, specifically any buffer allocated for response
 * content.
 */
void free_http_response(http_response* response);

/* To/from string conversion metods */
char* http_method_to_string(http_method method);
http_method string_to_http_method(char* method);
char* http_request_to_string(http_request* request, char* buf);
char* http_response_to_string(http_response* response, char* buf);

/* Parse an http_header from the buffer at header. The valid bit in the returned
 * header will be 0 if a header was not found or it was invalid.
 */
http_header parse_http_header(char* header);

/* Parse an http_uri from the buffer at uri. The valid bit in the returned
 * uri will be 0 if a URI was not found or it was invalid.
 */
http_uri parse_http_uri(char* uri);

/* Parse an http_request from the buffer at request. The valid bit in the
 * returned request will be 0 if a request was not found or it was invalid.
 *
 * Does not parse content in the buffer.
 */
http_request parse_http_request(char* request);

/* Parse an http_response from the buffer at request. The valid bit in the
 * returned response will be 0 if a response was not found or it was invalid.
 *
 * Does not parse content in the buffer.
 */
http_response parse_http_response(char* response);

/* Equality testing methods */
int equal_http_request(http_request* first, http_request* second);
int equal_http_uri(http_uri* first, http_uri* second);
int equal_http_headers(http_header* first, int first_count,
        http_header* second, int second_count);
int equal_http_message(http_message* first, http_message* second);

/* Slightly less strict equality testing requirements if we're checking
 * for a cached entry.
 */
int equal_cached_http_request(http_request* first, http_request* second);

#endif // _HTTP_H_
