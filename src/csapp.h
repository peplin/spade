/* $begin csapp.h */
#ifndef __CSAPP_H__
#define __CSAPP_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/* Default file permissions are DEF_MODE & ~DEF_UMASK */
/* $begin createmasks */
#define DEF_MODE   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
#define DEF_UMASK  S_IWGRP|S_IWOTH
/* $end createmasks */

/* Simplifies calls to bind(), connect(), and accept() */
/* $begin sockaddrdef */
typedef struct sockaddr SA;
/* $end sockaddrdef */

/* Persistent state for the robust I/O (Rio) package */
/* $begin rio_t */
#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                /* descriptor for this internal buf */
    int rio_cnt;               /* unread bytes in internal buf */
    char* rio_bufptr;          /* next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;
/* $end rio_t */

/* External variables */
extern int h_errno;    /* defined by BIND for DNS errors */ 
extern char **environ; /* defined by libc */

/* Misc constants */
#define	MAXLINE	 8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

/* Our own error-handling functions */
void unix_error(char* msg);
void posix_error(int code, char* msg);
void dns_error(char* msg);
void app_error(char* msg);

/* Process control wrappers */
pid_t csapp_fork(void);
void csapp_execve(const char* filename, char* const argv[], char* const envp[]);
pid_t csapp_wait(int* status);
pid_t csapp_waitpid(pid_t pid, int* iptr, int options);
void csapp_kill(pid_t pid, int signum);
unsigned int csapp_sleep(unsigned int secs);
void csapp_pause(void);
unsigned int csapp_alarm(unsigned int seconds);
void csapp_setpgid(pid_t pid, pid_t pgid);
pid_t csapp_getpgrp();

/* Signal wrappers */
typedef void handler_t(int);
handler_t* csapp_signal(int signum, handler_t* handler);
void csapp_sigprocmask(int how, const sigset_t* set, sigset_t* oldset);
void csapp_sigemptyset(sigset_t* set);
void csapp_sigfillset(sigset_t* set);
void csapp_sigaddset(sigset_t* set, int signum);
void csapp_sigdelset(sigset_t* set, int signum);
int csapp_sigismember(const sigset_t* set, int signum);

/* Unix I/O wrappers */
int csapp_open(const char* pathname, int flags, mode_t mode);
ssize_t csapp_read(int fd, void* buf, size_t count);
ssize_t csapp_write(int fd, const void* buf, size_t count);
off_t csapp_lseek(int fildes, off_t offset, int whence);
void csapp_close(int fd);
int csapp_select(int  n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, 
	   struct timeval* timeout);
int csapp_dup2(int fd1, int fd2);
void csapp_stat(const char* filename, struct stat* buf);
void csapp_fstat(int fd, struct stat* buf) ;

/* Memory mapping wrappers */
void* csapp_mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset);
void csapp_munmap(void* start, size_t length);

/* Standard I/O wrappers */
void csapp_fclose(FILE* fp);
FILE* csapp_fdopen(int fd, const char* type);
char* csapp_fgets(char* ptr, int n, FILE* stream);
FILE* csapp_fopen(const char* filename, const char* mode);
void csapp_fputs(const char* ptr, FILE* stream);
size_t csapp_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
void csapp_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);

/* Dynamic storage allocation wrappers */
void* csapp_malloc(size_t size);
void* csapp_realloc(void* ptr, size_t size);
void* csapp_calloc(size_t nmemb, size_t size);
void csapp_free(void* ptr);

/* Sockets interface wrappers */
int csapp_socket(int domain, int type, int protocol);
void csapp_setsockopt(int s, int level, int optname, const void* optval, int optlen);
void csapp_bind(int sockfd, struct sockaddr* my_addr, int addrlen);
void csapp_listen(int s, int backlog);
int csapp_accept(int s, struct sockaddr* addr, socklen_t* addrlen);
void csapp_connect(int sockfd, struct sockaddr* serv_addr, int addrlen);

/* DNS wrappers */
struct hostent* csapp_gethostbyname(const char* name);
struct hostent* csapp_gethostbyaddr(const char* addr, int len, int type);

/* Pthreads thread control wrappers */
void csapp_pthread_create(pthread_t* tidp, pthread_attr_t* attrp, 
		    void * (*routine)(void *), void* argp);
void csapp_pthread_join(pthread_t tid, void **thread_return);
void csapp_pthread_cancel(pthread_t tid);
void csapp_pthread_detach(pthread_t tid);
void csapp_pthread_exit(void* retval);
pthread_t csapp_pthread_self(void);
void csapp_pthread_once(pthread_once_t* once_control, void (*init_function)());

/* POSIX semaphore wrappers */
void csapp_sem_init(sem_t* sem, int pshared, unsigned int value);
void csapp_p(sem_t* sem);
void csapp_v(sem_t* sem);

/* Rio (Robust I/O) package */
ssize_t rio_readn(int fd, void* usrbuf, size_t n);
ssize_t rio_writen(int fd, void* usrbuf, size_t n);
void rio_readinitb(rio_t* rp, int fd); 
ssize_t	rio_readnb(rio_t* rp, void* usrbuf, size_t n);
ssize_t	rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen);

/* Wrappers for Rio package */
ssize_t csapp_rio_readn(int fd, void* usrbuf, size_t n);
void csapp_rio_writen(int fd, void* usrbuf, size_t n);
void csapp_rio_readinitb(rio_t* rp, int fd); 
ssize_t csapp_rio_readnb(rio_t* rp, void* usrbuf, size_t n);
ssize_t csapp_rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen);

/* Client/server helper functions */
int open_clientfd(char* hostname, int portno);
int open_listenfd(int portno);

/* Wrappers for client/server helper functions */
int csapp_open_clientfd(char* hostname, int port);
int csapp_open_listenfd(int port); 

#endif /* __CSAPP_H__ */
/* $end csapp.h */
