#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define INITIAL_BUFFER_SIZE 4096

// Standard Colors
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define WHITE "\x1b[37m"

// Style/Reset
#define BOLD "\x1b[1m"
#define RESET "\x1b[0m"


#define container_of(ptr, type, member) ((type *) ((char *) (ptr) - offsetof(type, member)))


static inline void die(const char *msg) {
    fprintf(stderr, "[%d] %s\n", errno, msg);
    exit(EXIT_FAILURE);
}

static inline void log_err(const char *msg) {
    fprintf(stderr, "[%d] %s\n", errno, msg);
}

static inline void log_errfn(const char *fnname, const char *msg) {
    fprintf(stderr, "[errno: ");
    fprintf(stderr, RED "%d" RESET, errno);
    fprintf(stderr, "] - [");
    fprintf(stderr, RED "%s" RESET, fnname);
    fprintf(stderr, "]: %s\n", msg);
}


static inline void printfn(const char *fnName) {
    printf("[");
    printf(CYAN BOLD "%s()" RESET, fnName);
    printf("]: ");
}

static inline bool str2int(const char *s, size_t *out, size_t n) {
    char *endp = NULL;
    *out = strtoll(s, &endp, 10);
    return endp == s + n;
}
