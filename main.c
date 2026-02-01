#include "./include/common.h"

#include "conn.h"
#include "downstream.h"
#include "pool.h"
#include "upstream.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>


#define MAX_UPSTREAM 16

static PoolArr *parr;
static size_t poolInd = 0;

static struct Config {
    char *hostip;
    int listen_port;
    int num_upstreams;
    int upstream_ports[MAX_UPSTREAM];
} config;


static void parse_options(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
        int lastArg = i == argc - 1;
        if (!(strcmp(argv[i], "-l")) && !lastArg) {
            config.listen_port = atoi(argv[++i]);
        } else if (!(strcmp(argv[i], "-p")) && !lastArg) {
            config.num_upstreams = 0;
            while (i + 1 < argc && argv[i + 1][0] != '-') {
                if (config.num_upstreams >= MAX_UPSTREAM) {
                    die("Too many upstream ports. Max allowed is 16\n");
                    break;
                }
                config.upstream_ports[config.num_upstreams++] = atoi(argv[++i]);
            }
        } else {
            die("Unknown argument");
        }
    }
    return;
}


static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }
    flags |= O_NONBLOCK;
    errno = 0;
    (void) fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}


int create_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return fd;
    }
    int val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
        return -1;
    }
    return fd;
}

int bind_socket(int fd) {
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(config.listen_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (const struct sockaddr *) &addr, sizeof(addr))) {
        return -1;
    }
    return 1;
}

char *copy_str(char *dest, char *src, size_t n) {
    if (n == 0) {
        return dest;
    }
    while (--n) {
        *dest = *src;
        if (*dest == '\0') {
            return dest;
        }
        dest++;
        src++;
    }
    *dest = '\0';
    return dest;
};

static void handle_write(Conn *conn) {
    Pool *pool = NULL;
    switch (conn->type) {
    case CONN_DOWNSTREAM:
        Downstream *ds = container_of(conn, Downstream, conn);
        if (!(pool = pget_ds(parr, ds))) {
            log_err("pget_ds() downstream does not exist\nhandle_write()\n");
            return;
        }
        if (conn_write(conn, pool->us->conn.fd) <= 0) {
            return;
        };
        break;
    case CONN_UPSTREAM:
        Upstream *us = container_of(conn, Upstream, conn);
        if (!(pool = pget_us(parr, us))) {
            log_err("pget_us() downstream does not exist\nhandle_write()\n");
            return;
        }
        // cannot pop
        Downstream *target_ds = ds_buf_pop(pool->ds_buf);
        if (conn_write(conn, target_ds->conn.fd) > 0) {
            // target_ds = ds_buf_pop(pool->ds_buf);
        }
        break;
    default:
        log_err("handle_write() no matching conn type");
        return;
    }
}

static void conn_destroy(Conn *conn) {
    switch (conn->type) {
    case CONN_DOWNSTREAM:
        Downstream *ds = container_of(conn, Downstream, conn);
        // printf("freed: ds with conn id: %d\n", ds->conn.fd);
        downstream_free(ds);
        return;
    case CONN_UPSTREAM:
        Upstream *us = container_of(conn, Upstream, conn);
        Pool *pool = pget_us(parr, us);
        if (pool) {
            parr_remove_us(parr, us);
        }
        upstream_free(us);
        return;
    default:
        log_err("conn_destroy() invalid conn to destroy\n");
        return;
    }
}


static void handle_read(Conn *conn) {
    if (conn_read(conn) > 0) {
        if (conn->type == CONN_DOWNSTREAM) {
            Pool *pool = parr->arr[poolInd];
            Downstream *ds = container_of(conn, Downstream, conn);
            if (!pool) {
                log_errfn("handle_read()", "Pool not found");
                return;
            }
            if (ds_buf_append(pool->ds_buf, ds) < 0) {
                log_err("[ ds_buf_append() ]\n[ handle_read() ] error\n");
                return;
            }
            poolInd = (poolInd + 1) % parr->size;
        }
    } else {
        conn_destroy(conn);
    }

    // write
    if (conn->status == WANT_WRITE) {
        return handle_write(conn);
    }
};

int main(int argc, char *argv[]) {
    config.hostip = "127.0.0.1";
    config.listen_port = 0;
    parse_options(argc, argv);

    if (!config.listen_port) {
        die("Listen port required\nUsage: -l [listen-port]\n");
    }
    if (!config.num_upstreams) {
        die("At least one upstream port required\nUsage: -p [upstream-port] [upstream-port]\n");
    }

    Conn *listener = create_conn(CONN_LISTENER);

    listener->fd = create_socket();

    if (listener->fd < 0) {
        die("create socket()");
    };

    if (bind_socket(listener->fd) < 0) {
        die("bind socket()");
    }

    fd_set_nb(listener->fd);

    if (listen(listener->fd, SOMAXCONN)) {
        die("listen()");
    }

    parr = parr_init(8);


    for (int i = 0; i < config.num_upstreams; ++i) {
        Upstream *ups = upstream_init(config.upstream_ports[i]);
        if (!ups) {
            continue;
        }
        Pool *pool = pool_init(ups);
        parr_append(parr, pool);
    }

    for (size_t i = 0; i < parr->size; ++i) {
        Upstream *us = parr->arr[i]->us;
        if (upstream_connect(us) < 0) {
            printf("upstream_connect() failed port: %d\n", parr->arr[i]->us->port);
            return 0;
        }
        if (us->flags & ALIVE) {
            printf("Server ");
            printf(GREEN "%zd" RESET, i + 1);
            printf(" alive on port:");
            printf(GREEN "%d" RESET, us->port);
            printf(", fd: %d\n", us->conn.fd);
        }
        fd_set_nb(us->conn.fd);
    }


    struct epoll_event ev, events[10];
    int nfds;
    int epfd = epoll_create(1);
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.ptr = listener;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listener->fd, &ev)) {
        die("epoll_ctl: listening sock");
    }

    for (size_t i = 0; i < parr->size; ++i) {
        Upstream *us = parr->arr[i]->us;
        if (upstream_connect(us) < 0) {
            die("upstream_connect()\n");
        }
        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.ptr = &us->conn;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, us->conn.fd, &ev) == -1) {
            printf("epoll_ctl: upstream port: %d\n", us->port);
        }
    }

    while (1) {
        nfds = epoll_wait(epfd, events, 10, -1);
        if (nfds == -1) {
            die("epoll_wait");
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.ptr == listener) {
                Downstream *ds = downstream_accept(listener->fd);
                if (!ds) {
                    die("accept() error");
                }
                fd_set_nb(ds->conn.fd);
                ev.events = EPOLLIN | EPOLLERR;
                ev.data.ptr = &ds->conn;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, ds->conn.fd, &ev) == -1) {
                    die("epoll_ctl: downstream conn sock");
                }
            } else {
                if (events[i].events & EPOLLIN) {
                    Conn *conn = (Conn *) events[i].data.ptr;
                    if (conn->status != WANT_READ)
                        continue;
                    handle_read(conn);
                } else if (events[i].events & EPOLLOUT) {
                    Conn *conn = (Conn *) events[i].data.ptr;
                    if (conn->status != WANT_WRITE)
                        continue;
                    handle_write(conn);
                } else if (events[i].events & EPOLLERR) {
                    Conn *conn = (Conn *) events[i].data.ptr;
                    if (conn->status != WANT_CLOSE)
                        continue;
                    conn_destroy(conn);
                }
            }
        }
    }
    return 0;
}
