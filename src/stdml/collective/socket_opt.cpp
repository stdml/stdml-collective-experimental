#include <cstdio>
#include <sys/socket.h>
#include <sys/types.h>

void set_default_native_socket_opts(int fd)
{
    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        fprintf(stderr, "%s\n", "setsockopt(SO_REUSEADDR) failed");
    }
}
