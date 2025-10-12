#include "downstream.h"
#include "buffer.h"
#include "common.h"
#include "conn.h"

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

Downstream *downstream_accept(int fd) {
    struct sockaddr_in downstream_addr = {};
    socklen_t socklen = sizeof(downstream_addr);
    int connfd = accept(fd, (struct sockaddr *) &downstream_addr, &socklen);
    if (connfd < 0) {
        return NULL;
    }
    uint32_t ip = ntohl(downstream_addr.sin_addr.s_addr);
    fprintf(stderr,
            "new client from %u.%u.%u.%u:%u\n",
            ip & 255,
            (ip >> 8) & 255,
            (ip >> 16) & 255,
            ip >> 24,
            ntohs(downstream_addr.sin_port));
    Downstream *ds = malloc(sizeof(Downstream));
    ds->readBuffer = buf_init();
    ds->writeBuffer = buf_init();
    if (!ds) {
        close(connfd);
        return NULL;
    }
    ds->conn.fd = connfd;
    ds->conn.type = CONN_DOWNSTREAM;
    ds->conn.status = WANT_READ;
    return ds;
}

void downstream_free(Downstream *ds) {
    close(ds->conn.fd);
    buf_free(ds->readBuffer);
    buf_free(ds->writeBuffer);
    free(ds);
};


DownstreamBuffer *ds_buf_init() {
    DownstreamBuffer *ds_buf = malloc(sizeof(DownstreamBuffer));
    if (!ds_buf) {
        fprintf(stderr, "Failed to allocate DownstreamArr");
        return NULL;
    }
    ds_buf->arr = malloc(INITIAL_DOWNSTREAM_ARRAY_CAPACITY * sizeof(DownstreamBuffer *));
    if (!ds_buf->arr) {
        free(ds_buf);
        fprintf(stderr, "Failed to allocate DownstreamArr.arr");
        return NULL;
    }
    ds_buf->head = ds_buf->tail = 0;
    ds_buf->cap = INITIAL_DOWNSTREAM_ARRAY_CAPACITY;
    return ds_buf;
}

int ds_arr_realloc(DownstreamBuffer *ds_buf, size_t n) {
    Downstream **tmp = malloc(ds_buf->cap * sizeof(DownstreamBuffer *));
    if (!tmp) {
        return -1;
    }
    size_t tmpInd = 0;
    size_t buf_tail = ds_buf->tail;
    while (ds_buf->tail < ds_buf->cap) {
        tmp[tmpInd++] = ds_buf->arr[ds_buf->tail++];
    }
    while (ds_buf->head < buf_tail) {
        tmp[tmpInd++] = ds_buf->arr[ds_buf->head++ % ds_buf->cap];
    }
    free(ds_buf->arr);
    ds_buf->arr = realloc(tmp, n);
    ds_buf->cap = n;
    ds_buf->tail = 0;
    ds_buf->head = ds_buf->cap - 1;
    return 1;
}

int ds_buf_append(DownstreamBuffer *ds_buf, Downstream *ds) {
    if (((ds_buf->head + 1) % ds_buf->cap) == ds_buf->tail) {
        if (!ds_arr_realloc(ds_buf, ds_buf->cap * 2)) {
            return -1;
        }
    }
    ds_buf->arr[ds_buf->head] = ds;
    ds_buf->head = (ds_buf->head + 1) % ds_buf->cap;
    return 1;
}

Downstream *ds_buf_pop(DownstreamBuffer *ds_buf) {
    if (ds_buf->tail == ds_buf->head) {
        return NULL;
    }
    Downstream *ds = ds_buf->arr[ds_buf->tail];
    ds_buf->tail = (ds_buf->tail + 1) % ds_buf->cap;
    return ds;
}


void ds_buf_free(DownstreamBuffer *ds_buf) {
    for (size_t i = 0; i < ds_buf->cap; ++i) {
        downstream_free(ds_buf->arr[i]);
    }
    free(ds_buf->arr);
}

int ds_buf_exists(DownstreamBuffer *ds_buf, Downstream *ds) {
    size_t temp = ds_buf->tail;
    while (temp != ds_buf->head) {
        if (ds_buf->arr[temp] == ds) {
            return 1;
        }
        temp = (temp + 1) % ds_buf->cap;
    }
    return -1;
}
