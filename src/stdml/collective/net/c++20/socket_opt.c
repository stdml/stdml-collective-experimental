#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

static void reuse_addr(int fd)
{
    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
}

static void no_delay(int fd)
{
    int enable = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
        perror("setsockopt(TCP_NODELAY) failed");
    }
}

static void show_socket_opts(int fd)
{
    int old = -1;
    socklen_t len = sizeof(int);
    if (getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &old, &len) < 0) {
        perror("getsockopt(TCP_NODELAY) failed");
    }
    // old getsockopt(TCP_NODELAY)=0, len=4
    fprintf(stderr, "old getsockopt(TCP_NODELAY)=%d, len=%d\n", old, (int)len);
}

void set_default_server_socket_opts(int fd)
{
    reuse_addr(fd);
}

void set_default_client_socket_opts(int fd)
{
    // show_socket_opts(fd);
    // no_delay(fd);
}
