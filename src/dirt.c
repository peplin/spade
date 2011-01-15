/* simple libev-based http server */
#include <ev.h>
#include <log4c.h>

#include "csapp.h"
#include "http.h"
#include "util.h"

static log4c_category_t* logger = NULL;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
         char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    if(log4c_init()) {
        printf("log4c init failed");
        exit(1);
    }
    logger = log4c_category_get("dirt");

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);
    log4c_category_log(logger, LOG4C_PRIORITY_INFO,
            "Starting dirt web server on port %d", port);

    listenfd = csapp_open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = csapp_accept(listenfd, (SA *)&clientaddr,
                        (socklen_t*)&clientlen);
        doit(connfd);
        csapp_close(connfd);
    }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd) {
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    csapp_rio_readinitb(&rio, fd);
    csapp_rio_readlineb(&rio, buf, MAXLINE);                   
    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET")) {                     
       clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
        return;
    }                                                    
    read_requesthdrs(&rio);                             

    /* Parse URI from GET request */
    http_uri parsed_uri = parse_http_uri(uri);
    if (stat(parsed_uri.path, &sbuf) < 0) {                     
        clienterror(fd, parsed_uri.path, "404", "Not found",
                "Tiny couldn't find this file");
        return;
    }                                                    

    if (parsed_uri.is_dynamic) { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { 
            clienterror(fd, parsed_uri.path, "403", "Forbidden",
                "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, parsed_uri.path, parsed_uri.query_string);            
    } else { /* Serve static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { 
            clienterror(fd, parsed_uri.path, "403", "Forbidden",
                "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, parsed_uri.path, sbuf.st_size);        
    }
}

/*
 * read_requesthdrs - read and parse HTTP request headers
 */
void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    csapp_rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {          
    csapp_rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    }
    return;
}

/*
 * serve_static - copy a file back to the client
 */
void serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);       
    sprintf(buf, "HTTP/1.0 200 OK\r\n");    
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    csapp_rio_writen(fd, buf, strlen(buf));       

    /* Send response body to client */
    srcfd = csapp_open(filename, O_RDONLY, 0);    
    srcp = csapp_mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    csapp_close(srcfd);                           
    csapp_rio_writen(fd, srcp, filesize);         
    csapp_munmap(srcp, filesize);                 
}

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
void serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    csapp_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    csapp_rio_writen(fd, buf, strlen(buf));

    if (csapp_fork() == 0) { /* child */ 
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); 
    csapp_dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ 
    csapp_execve(filename, emptylist, environ); /* Run CGI program */ 
    }
    csapp_wait(NULL); /* Parent waits for and reaps child */
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
         char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    csapp_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    csapp_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    csapp_rio_writen(fd, buf, strlen(buf));
    csapp_rio_writen(fd, body, strlen(body));
}
