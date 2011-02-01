/*
 * adder.c - a minimal Clay program that adds two numbers together
 */

#include <zmq.h>

#include "csapp.h"
#include "../../src/clay.h"

// TODO inthe future, can use ZMQ_SNDMORE to allow bigger responses

void adder(void* socket, clay_variables variables) {
    int first = 0, second = 0;
    sscanf(variables.query_string, "value=%d&value=%d", &first, &second);


    char content[MAXLINE];
    sprintf(content, "%d\r\n", first + second);

    char buf[MAX_RESPONSE_SIZE];
    sprintf(buf, "Content-Type: text/html\r\n");
    sprintf(buf, "%sContent-Length: %zu\r\n\r\n", buf, strlen(content));
    sprintf(buf, "%s%s", buf, content);

    clay_response response;
    response.incoming_socket = variables.incoming_socket;
    response.response_length = strlen(buf);
    memcpy(response.response, buf, response.response_length);

    zmq_msg_t msg;
    zmq_msg_init_data(&msg, &response, sizeof(clay_response), NULL, NULL);
    zmq_send(socket, &msg, 0);
}

int main(void) {
    zmq_msg_t msg;
    zmq_msg_init (&msg);

    void* zmq_context = zmq_init(10);
    void* socket = zmq_socket(zmq_context, ZMQ_PAIR);
    int rc = zmq_connect(socket, "ipc:///tmp/adder.sock");
    clay_variables variables;

    while(1) {
        rc = zmq_recv(socket, &msg, 0);
        memcpy(&variables, zmq_msg_data(&msg), zmq_msg_size(&msg));
        adder(socket, variables);
    }
}
