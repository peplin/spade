#ifndef _DIRT_H_
#define _DIRT_H_

#include <ev.h>
#include <log4c.h>

#include "csapp.h"
#include "http.h"
#include "util.h"
#include "server.h"

#define DEFAULT_PORT 8080

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void serve_static(int fd, char *filename, int filesize);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
         char *shortmsg, char *longmsg);

#endif // _DIRT_H_
