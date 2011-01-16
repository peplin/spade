#include <log4c.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "util.h"

int check_error(int result, const char* function, pthread_mutex_t* mutex, 
        int verbosity) {
    if(result < 0) {
        if(verbosity) {
            int err = errno;
            log4c_category_log(log4c_category_get("dirt"), LOG4C_PRIORITY_WARN,
                    "ERROR: %s failed with error %d: %s\n", function, errno,
                    strerror(err));
        }
        return -1;
    }
    return 0;
}

void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html")) {
        strcpy(filetype, "text/html");
    } else if (strstr(filename, ".gif")) {
        strcpy(filetype, "image/gif");
    } else if (strstr(filename, ".jpg")) {
        strcpy(filetype, "image/jpeg");
    } else {
        strcpy(filetype, "text/plain");
    }
}

