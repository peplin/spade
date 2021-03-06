/* $begin csapp.c */
#include "csapp.h"

/************************** 
 * Error-handling functions
 **************************/
/* $begin errorfuns */
/* $begin unixerror */
void unix_error(char* msg) /* unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

void posix_error(int code, char* msg) /* posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void dns_error(char* msg) /* dns-style error */
{
    fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
    exit(0);
}

void app_error(char* msg) /* application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
/* $end errorfuns */

/*********************************************
 * Wrappers for Unix process control functions
 ********************************************/

/* $begin forkwrapper */
pid_t csapp_fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("fork error");
    return pid;
}
/* $end forkwrapper */

void csapp_execve(const char* filename, char* const argv[], char* const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	unix_error("execve error");
}

/* $begin wait */
pid_t csapp_wait(int* status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("wait error");
    return pid;
}
/* $end wait */

pid_t csapp_waitpid(pid_t pid, int* iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("waitpid error");
    return(retpid);
}

/* $begin kill */
void csapp_kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("kill error");
}
/* $end kill */

void csapp_pause() 
{
    (void)pause();
    return;
}

unsigned int csapp_sleep(unsigned int secs) 
{
    unsigned int rc;

    if ((rc = sleep(secs)) < 0)
	unix_error("sleep error");
    return rc;
}

unsigned int csapp_alarm(unsigned int seconds) {
    return alarm(seconds);
}
 
void csapp_setpgid(pid_t pid, pid_t pgid) {
    int rc;

    if ((rc = setpgid(pid, pgid)) < 0)
	unix_error("setpgid error");
    return;
}

pid_t csapp_getpgrp(void) {
    return getpgrp();
}

/************************************
 * Wrappers for Unix signal functions 
 ***********************************/

/* $begin sigaction */
handler_t* csapp_signal(int signum, handler_t* handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("signal error");
    return (old_action.sa_handler);
}
/* $end sigaction */

void csapp_sigprocmask(int how, const sigset_t* set, sigset_t* oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("sigprocmask error");
    return;
}

void csapp_sigemptyset(sigset_t* set)
{
    if (sigemptyset(set) < 0)
	unix_error("sigemptyset error");
    return;
}

void csapp_sigfillset(sigset_t* set)
{ 
    if (sigfillset(set) < 0)
	unix_error("sigfillset error");
    return;
}

void csapp_sigaddset(sigset_t* set, int signum)
{
    if (sigaddset(set, signum) < 0)
	unix_error("sigaddset error");
    return;
}

void csapp_sigdelset(sigset_t* set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("sigdelset error");
    return;
}

int csapp_sigismember(const sigset_t* set, int signum)
{
    int rc;
    if ((rc = sigismember(set, signum)) < 0)
	unix_error("sigismember error");
    return rc;
}


/********************************
 * Wrappers for Unix I/O routines
 ********************************/

int csapp_open(const char* pathname, int flags, mode_t mode) 
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
	unix_error("open error");
    return rc;
}

ssize_t csapp_read(int fd, void* buf, size_t count) 
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0) 
	unix_error("read error");
    return rc;
}

ssize_t csapp_write(int fd, const void* buf, size_t count) 
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
	unix_error("write error");
    return rc;
}

off_t csapp_lseek(int fildes, off_t offset, int whence) 
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
	unix_error("lseek error");
    return rc;
}

void csapp_close(int fd) 
{
    int rc;

    if ((rc = close(fd)) < 0)
	unix_error("close error");
}

int csapp_select(int  n, fd_set* readfds, fd_set* writefds,
	   fd_set* exceptfds, struct timeval* timeout) 
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
	unix_error("select error");
    return rc;
}

int csapp_dup2(int fd1, int fd2) 
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
	unix_error("dup2 error");
    return rc;
}

void csapp_stat(const char* filename, struct stat* buf) 
{
    if (stat(filename, buf) < 0)
	unix_error("stat error");
}

void csapp_fstat(int fd, struct stat* buf) 
{
    if (fstat(fd, buf) < 0)
	unix_error("fstat error");
}

/***************************************
 * Wrappers for memory mapping functions
 ***************************************/
void* csapp_mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset) 
{
    void* ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
	unix_error("mmap error");
    return(ptr);
}

void csapp_munmap(void* start, size_t length) 
{
    if (munmap(start, length) < 0)
	unix_error("munmap error");
}

/***************************************************
 * Wrappers for dynamic storage allocation functions
 ***************************************************/

void* csapp_malloc(size_t size) 
{
    void* p;

    if ((p  = malloc(size)) == NULL)
	unix_error("malloc error");
    return p;
}

void* csapp_realloc(void* ptr, size_t size) 
{
    void* p;

    if ((p  = realloc(ptr, size)) == NULL)
	unix_error("realloc error");
    return p;
}

void* csapp_calloc(size_t nmemb, size_t size) 
{
    void* p;

    if ((p = calloc(nmemb, size)) == NULL)
	unix_error("calloc error");
    return p;
}

void csapp_free(void* ptr) 
{
    free(ptr);
}

/******************************************
 * Wrappers for the Standard I/O functions.
 ******************************************/
void csapp_fclose(FILE* fp) 
{
    if (fclose(fp) != 0)
	unix_error("fclose error");
}

FILE* csapp_fdopen(int fd, const char* type) 
{
    FILE* fp;

    if ((fp = fdopen(fd, type)) == NULL)
	unix_error("csapp_fdopen error");

    return fp;
}

char* csapp_fgets(char* ptr, int n, FILE* stream) 
{
    char* rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
	app_error("fgets error");

    return rptr;
}

FILE* Fopen(const char* filename, const char* mode) 
{
    FILE* fp;

    if ((fp = fopen(filename, mode)) == NULL)
	unix_error("fopen error");

    return fp;
}

void csapp_fputs(const char* ptr, FILE* stream) 
{
    if (fputs(ptr, stream) == EOF)
	unix_error("fputs error");
}

size_t csapp_fread(void* ptr, size_t size, size_t nmemb, FILE* stream) 
{
    size_t n;

    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream)) 
	unix_error("fread error");
    return n;
}

void csapp_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream) 
{
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
	unix_error("fwrite error");
}


/**************************** 
 * Sockets interface wrappers
 ****************************/

int csapp_socket(int domain, int type, int protocol) 
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
	unix_error("socket error");
    return rc;
}

void csapp_setsockopt(int s, int level, int optname, const void* optval, int optlen) 
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
	unix_error("setsockopt error");
}

void csapp_bind(int sockfd, struct sockaddr* my_addr, int addrlen) 
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
	unix_error("bind error");
}

void csapp_listen(int s, int backlog) 
{
    int rc;

    if ((rc = listen(s,  backlog)) < 0)
	unix_error("listen error");
}

int csapp_accept(int s, struct sockaddr* addr, socklen_t* addrlen) 
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
	unix_error("accept error");
    return rc;
}

void csapp_connect(int sockfd, struct sockaddr* serv_addr, int addrlen) 
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
	unix_error("connect error");
}

/************************
 * DNS interface wrappers 
 ***********************/

/* $begin gethostbyname */
struct hostent* csapp_gethostbyname(const char* name) 
{
    struct hostent* p;

    if ((p = gethostbyname(name)) == NULL)
	dns_error("gethostbyname error");
    return p;
}
/* $end gethostbyname */

struct hostent* csapp_gethostbyaddr(const char* addr, int len, int type) 
{
    struct hostent* p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
	dns_error("gethostbyaddr error");
    return p;
}

/************************************************
 * Wrappers for Pthreads thread control functions
 ************************************************/

void csapp_pthread_create(pthread_t* tidp, pthread_attr_t* attrp, 
		    void * (*routine)(void *), void* argp) 
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
	posix_error(rc, "pthread_create error");
}

void csapp_pthread_cancel(pthread_t tid) {
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
	posix_error(rc, "pthread_cancel error");
}

void csapp_pthread_join(pthread_t tid, void **thread_return) {
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
	posix_error(rc, "pthread_join error");
}

/* $begin detach */
void csapp_pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
	posix_error(rc, "pthread_detach error");
}
/* $end detach */

void csapp_pthread_exit(void* retval) {
    pthread_exit(retval);
}

pthread_t csapp_pthread_self(void) {
    return pthread_self();
}
 
void csapp_pthread_once(pthread_once_t* once_control, void (*init_function)()) {
    pthread_once(once_control, init_function);
}

/*******************************
 * Wrappers for Posix semaphores
 *******************************/

void csapp_sem_init(sem_t* sem, int pshared, unsigned int value) 
{
    if (sem_init(sem, pshared, value) < 0)
	unix_error("sem_init error");
}

void csapp_p(sem_t* sem) 
{
    if (sem_wait(sem) < 0)
	unix_error("p error");
}

void csapp_v(sem_t* sem) 
{
    if (sem_post(sem) < 0)
	unix_error("v error");
}

/*********************************************************************
 * The Rio package - robust I/O functions
 **********************************************************************/
/*
 * rio_readn - robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void* usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* interrupted by sig handler return */
		nread = 0;      /* and call read() again */
	    else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void* usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char* bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* interrupted by sig handler return */
		nwritten = 0;    /* and call write() again */
	    else
		return -1;       /* errorno set by write() */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t* rp, char* usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* refill if buf is empty */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* interrupted by sig handler return */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else 
	    rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->rio_cnt < n)   
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t* rp, int fd) 
{
    rp->rio_fd = fd;  
    rp->rio_cnt = 0;  
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t* rp, void* usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;
    
    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* interrupted by sig handler return */
		nread = 0;      /* call read() again */
	    else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/* 
 * rio_readlineb - robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen) 
{
    int n, rc;
    char c,* bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
	if ((rc = rio_read(rp, &c, 1)) == 1) {
	   * bufp++ = c;
	    if (c == '\n')
		break;
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* error */
    }
   * bufp = 0;
    return n;
}
/* $end rio_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t csapp_rio_readn(int fd, void* ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
	unix_error("rio_readn error");
    return n;
}

void csapp_rio_writen(int fd, void* usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n)
	unix_error("rio_writen error");
}

void csapp_rio_readinitb(rio_t* rp, int fd)
{
    rio_readinitb(rp, fd);
} 

ssize_t csapp_rio_readnb(rio_t* rp, void* usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
	unix_error("rio_readnb error");
    return rc;
}

ssize_t csapp_rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
	unix_error("rio_readlineb error");
    return rc;
} 

/******************************** 
 * Client/server helper functions
 ********************************/
/*
 * open_clientfd - open connection to server at <hostname, port> 
 *   and return a socket descriptor ready for reading and writing.
 *   Returns -1 and sets errno on Unix error. 
 *   Returns -2 and sets h_errno on DNS (gethostbyname) error.
 */
/* $begin open_clientfd */
int open_clientfd(char* hostname, int port) 
{
    int clientfd;
    struct hostent* hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
	return -2; /* check h_errno for cause of error */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], 
	  (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
	return -1;
    return clientfd;
}
/* $end open_clientfd */

/*  
 * open_listenfd - open and return a listening socket on port
 *     Returns -1 and sets errno on Unix error.
 */
/* $begin open_listenfd */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	return -1;
 
    /* Eliminates "address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
		   (const void *)&optval , sizeof(int)) < 0)
	return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any csapp_iP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
	return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}
/* $end open_listenfd */

/******************************************
 * Wrappers for the client/server helper routines 
 ******************************************/
int csapp_open_clientfd(char* hostname, int port) 
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0) {
	if (rc == -1)
	    unix_error("open_clientfd Unix error");
	else        
	    dns_error("open_clientfd DNS error");
    }
    return rc;
}

int csapp_open_listenfd(int port) 
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
	unix_error("open_listenfd error");
    return rc;
}
/* $end csapp.c */




