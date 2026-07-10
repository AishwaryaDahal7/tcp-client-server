#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd, client_fd, i;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int client_fds[MAX_CLIENTS] = {0};
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);
    printf("Non-blocking I/O server on port %d\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_fd >= 0) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, ip, INET_ADDRSTRLEN);
            printf("New client: %s:%d\n", ip, ntohs(address.sin_port));
            set_nonblocking(client_fd);
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == 0) { client_fds[i] = client_fd; break; }
            }
        } else if (errno != EWOULDBLOCK) {
            perror("accept");
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_fds[i];
            if (sd == 0) continue;
            ssize_t n = read(sd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("fd %d: %s\n", sd, buffer);
                write(sd, buffer, n);
            } else if (n == 0 || (n < 0 && errno != EWOULDBLOCK)) {
                close(sd);
                client_fds[i] = 0;
                printf("fd %d disconnected\n", sd);
            }
        }
        usleep(10000);
    }

    close(server_fd);
    return 0;
}
