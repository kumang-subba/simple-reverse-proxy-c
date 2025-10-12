#include "buffer.h"
#include "include/common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Buffer *buf_init() {
    Buffer *buf = malloc(sizeof(Buffer));
    if (!buf) {
        return NULL;
    }
    buf->buffer_begin = malloc(INITIAL_BUFFER_SIZE * sizeof(buf->buffer_begin));
    if (!buf->buffer_begin) {
        return NULL;
    }
    buf->data_begin = buf->data_end = buf->buffer_begin;
    buf->buffer_end = buf->buffer_begin + INITIAL_BUFFER_SIZE;
    return buf;
}

void buf_free(Buffer *buf) {
    if (!buf) {
        return;
    }
    if (buf->buffer_begin) {
        free(buf->buffer_begin);
        buf->buffer_begin = NULL;
        buf->data_begin = NULL;
        buf->data_end = NULL;
        buf->buffer_end = NULL;
    }
    free(buf);
}

void buf_reset(Buffer *buf) {
    if (!buf || !buf->buffer_begin) {
        return;
    }
    buf->data_begin = buf->buffer_begin;
    buf->data_end = buf->buffer_begin;
    buf->buffer_end = buf->buffer_begin + (buf->buffer_end - buf->buffer_begin);
}

static int buf_realloc(Buffer *buf, size_t n) {
    size_t used = buf->data_end - buf->buffer_begin;
    size_t buf_size = buf->buffer_end - buf->buffer_begin;
    size_t new_size = buf_size;
    while ((new_size - used) < n) {
        new_size *= 2;
    }
    size_t data_size = buf->data_end - buf->data_begin;
    uint8_t *new_buf = malloc(new_size);
    if (!new_buf)
        return -1;
    memcpy(new_buf, buf->data_begin, data_size);
    free(buf->buffer_begin);
    buf->buffer_begin = new_buf;
    buf->buffer_end = new_buf + new_size;
    buf->data_begin = new_buf;
    buf->data_end = new_buf + data_size;
    printf("new size %zu\n", n);
    return 1;
}

int buf_append(Buffer *buf, const uint8_t *data, size_t n) {
    size_t spaceEnd = buf->buffer_end - buf->data_end;
    if (n > spaceEnd) {
        size_t sizeStart = buf->data_begin - buf->buffer_begin;
        if (sizeStart < n) {
            size_t dataSize = buf->data_end - buf->data_begin;
            memcpy(buf->buffer_begin, buf->data_begin, dataSize);
            buf->data_begin = buf->buffer_begin;
            buf->data_end = buf->data_begin + dataSize;
        } else {
            if (!buf_realloc(buf, n)) {
                return -1;
            }
        }
    }
    memcpy(buf->data_end, data, n);
    buf->data_end += n;
    return 1;
}


void buf_consume(Buffer *buf, size_t n) {
    buf->data_begin += n;
}
