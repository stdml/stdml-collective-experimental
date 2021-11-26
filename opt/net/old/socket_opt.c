#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

void reuse_addr(int fd)
{
    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }
}

void set_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(fd, F_GETFL, 0) failed");
    }
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
