#pragma once

#include "buffer.h"
#include "conn.h"

#include <stdint.h>
#include <sys/types.h>

#define INITIAL_DOWNSTREAM_ARRAY_CAPACITY (1 << 7)

typedef struct Downstream {
    char host[16];
    Conn conn;
    Buffer *readBuffer;
    Buffer *writeBuffer;
} Downstream;

Downstream *downstream_accept(int fd);
void downstream_free(Downstream *ds);

typedef struct DownstreamBuffer {
    Downstream **arr;
    size_t head;
    size_t tail;
    size_t cap;
} DownstreamBuffer;

DownstreamBuffer *ds_buf_init();
int ds_buf_append(DownstreamBuffer *ds_buf, Downstream *ds);
Downstream *ds_buf_pop(DownstreamBuffer *ds_buf);
void ds_buf_free(DownstreamBuffer *ds_buf);
int ds_buf_exists(DownstreamBuffer *ds_buf, Downstream *ds);
