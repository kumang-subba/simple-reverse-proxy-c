#include "upstream.h"

#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Upstream *upstream_init(int port) {
    Upstream *ups = malloc(sizeof(Upstream));
    ups->port = port;
    ups->readBuffer = buf_init();
    ups->writeBuffer = buf_init();
    ups->flags |= CONNECTING;
    return ups;
}

int upstream_connect(Upstream *us) {
    us->conn.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (us->conn.fd < 0)
        return -1;
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(us->port);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    if (connect(us->conn.fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        close(us->conn.fd);
        return -1;
    }
    us->conn.status = WANT_READ;
    us->conn.type = CONN_UPSTREAM;
    us->flags |= ALIVE;
    return 1;
}

void upstream_free(Upstream *up) {
    close(up->conn.fd);
    buf_free(up->readBuffer);
    buf_free(up->writeBuffer);
    free(up);
}
