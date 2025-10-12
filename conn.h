#pragma once

#include <sys/types.h>

typedef enum ConnType { CONN_LISTENER, CONN_DOWNSTREAM, CONN_UPSTREAM } ConnType;

typedef enum ConnStatus { WANT_READ, WANT_WRITE, WANT_CLOSE } ConnStatus;

typedef struct Conn {
    int fd;
    ConnType type;
    ConnStatus status;
} Conn;

Conn *create_conn(ConnType type);

int conn_write(Conn *conn, int target_fd);
int conn_read(Conn *conn);
