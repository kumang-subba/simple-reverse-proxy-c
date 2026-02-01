#include "conn.h"
#include "buffer.h"
#include "downstream.h"
#include "include/common.h"
#include "upstream.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define LF (u_char) '\n'
#define CR (u_char) '\r'
#define CRLF "\r\n"


#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>

Conn *create_conn(ConnType type) {
    Conn *conn = malloc(sizeof(Conn));
    conn->fd = -1;
    conn->type = type;
    return conn;
}

static void conn_buf_append(Conn *conn, uint8_t *data, size_t n) {
    switch (conn->type) {
    case CONN_DOWNSTREAM:
        Downstream *ds = container_of(conn, Downstream, conn);
        buf_append(ds->readBuffer, data, n);
        break;
    case CONN_UPSTREAM:
        Upstream *us = container_of(conn, Upstream, conn);
        buf_append(us->readBuffer, data, n);
        break;
    default:
        die("con_buf_append() error");
    }
}

static size_t parse_req(Buffer *buf) {
    size_t i = 0;
    size_t cl_str_start = 0;
    size_t cl_str_size = 0;
    size_t buf_size = buf->data_end - buf->data_begin;
    size_t hl = 0;
    size_t cl = 0;
    while (i < buf_size) {
        if (memcmp((buf->data_begin + i), "Content-Length: ", 16) == 0) {
            i += 16;
            cl_str_start = i;
            while (i < buf_size && ((memcmp(buf->data_begin + i, CRLF, 2)) != 0)) {
                cl_str_size++;
                i++;
            }
            char cl_buf[32];
            memcpy(cl_buf, buf->data_begin + cl_str_start, cl_str_size);
            cl_buf[cl_str_size] = '\0';
            if (!str2int(cl_buf, &cl, cl_str_size)) {
                die("str2int()");
            }
        }
        if (memcmp(buf->data_begin + i, CRLF CRLF, 4) == 0) {
            hl = i + 4;
            if (buf_size < (hl + cl)) {
                return -1;
            }
            return hl + cl;
        }
        i++;
    }
    return -1;
}

static int check_req(Conn *conn) {
    Buffer *readBuf = NULL;
    Buffer *writeBuf = NULL;
    switch (conn->type) {
    case CONN_DOWNSTREAM:
        Downstream *ds = container_of(conn, Downstream, conn);
        readBuf = ds->readBuffer;
        writeBuf = ds->writeBuffer;
        break;
    case CONN_UPSTREAM:
        Upstream *us = container_of(conn, Upstream, conn);
        readBuf = us->readBuffer;
        writeBuf = us->writeBuffer;
        break;
    default:
        die("check_req()");
    }
    if (!readBuf) {
        return -1;
    }
    size_t reqLen = parse_req(readBuf);
    if (reqLen <= 0) {
        return -1;
    }
    buf_append(writeBuf, readBuf->data_begin, reqLen);
    buf_consume(readBuf, reqLen);
    return 1;
}

int conn_write(Conn *conn, int target_fd) {
    Buffer *writeBuf = NULL;
    switch (conn->type) {
    case CONN_DOWNSTREAM:
        Downstream *ds = container_of(conn, Downstream, conn);
        writeBuf = ds->writeBuffer;
        break;
    case CONN_UPSTREAM:
        Upstream *us = container_of(conn, Upstream, conn);
        writeBuf = us->writeBuffer;
        break;
    default:
        die("check_req()");
    }
    if (!writeBuf) {
        return -1;
    }
    size_t reqLen = parse_req(writeBuf);
    if (reqLen <= 0) {
        return -1;
    }

    ssize_t wv = write(target_fd, writeBuf->data_begin, reqLen);
    if (wv < 0 && errno == EAGAIN) {
        if (errno == EAGAIN) {
            return -1;
        } else {
            log_err("write() error");
            conn->status = WANT_CLOSE;
            return 0;
        }
    }

    buf_consume(writeBuf, reqLen);
    if (writeBuf->data_end != writeBuf->data_begin) {
        return -1;
    }
    conn->status = WANT_READ;
    return 1;
}

int conn_read(Conn *conn) {
    uint8_t buf[64 * 1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));
    if (rv < 0 && errno == EAGAIN) {

        return -1;
    }
    if (rv < 0) {
        log_err("read() error");
        conn->status = WANT_CLOSE;
        return -1;
    }
    if (rv == 0) {
        // log_err("client closed or EOF");
        conn->status = WANT_CLOSE;
        return -1;
    }

    conn_buf_append(conn, buf, rv);

    if (check_req(conn) > 0) {
        conn->status = WANT_WRITE;
        return 1;
    }
    return 0;
}
