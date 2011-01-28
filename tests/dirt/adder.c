#include <stdio.h>

#include "../../src/dirt.h"
#include "../../src/csapp.h"

void adder(int incoming_socket, dirt_variables variables) {
    int first = 0, second = 0;
    sscanf(variables.query_string, "value=%d&value=%d", &first, &second);

    char content[MAXLINE];
    sprintf(content, "%d\r\n", first + second);

    char buf[MAXLINE];
    sprintf(buf, "Content-Type: text/html\r\n");
    if(rio_writen(incoming_socket, buf, strlen(buf)) == -1) {
        return;
    }

    sprintf(buf, "Content-Length: %zu\r\n\r\n", strlen(content));
    if(rio_writen(incoming_socket, buf, strlen(buf)) == -1) {
        return;
    }

    rio_writen(incoming_socket, content, strlen(content));
}
