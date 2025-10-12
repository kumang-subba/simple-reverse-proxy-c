#pragma once

#include <stdint.h>
#include <sys/types.h>

typedef struct Buffer {
    uint8_t *buffer_begin;
    uint8_t *buffer_end;
    uint8_t *data_begin;
    uint8_t *data_end;
} Buffer;


Buffer *buf_init();
void buf_free(Buffer *buf);
int buf_append(Buffer *buf, const uint8_t *data, size_t n);
void buf_consume(Buffer *buf, size_t n);
void buf_reset(Buffer *buf);
