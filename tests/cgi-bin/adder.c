/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
    int first = 0, second = 0;
    sscanf(getenv("QUERY_STRING"), "value=%d&value=%d", &first, &second);

    char content[MAXLINE];
    sprintf(content, "%d\r\n", first + second);

    /* Generate the HTTP response */
    printf("Content-Length: %zu\r\n", strlen(content));
    printf("Content-Type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    exit(0);
}
/* $end adder */
