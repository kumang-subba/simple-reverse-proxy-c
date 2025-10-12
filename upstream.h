#pragma once

#include "buffer.h"
#include "conn.h"

#include <sys/types.h>

#define DEAD 0x00
#define ALIVE 0x01
#define CONNECTING 0x02
#define DISCONNECTING 0x04

#define INITIAL_UPSTREAM_ARRAY_CAPACITY (1 << 2)

typedef struct Upstream {
    char host[16];
    int port;
    Conn conn;
    unsigned char flags;
    Buffer *readBuffer;
    Buffer *writeBuffer;
} Upstream;

Upstream *upstream_init(int port);
int upstream_connect(Upstream *up);
void upstream_free(Upstream *up);
