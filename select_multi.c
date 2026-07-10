#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd, max_fd, i;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    fd_set readfds, all_fds;
    int client_fds[FD_SETSIZE];
    char buffer[BUFFER_SIZE];

    for (i = 0; i < FD_SETSIZE; i++)
        client_fds[i] = -1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(server_fd, 5) < 0) { perror("listen"); exit(1); }

    FD_ZERO(&all_fds);
    FD_SET(server_fd, &all_fds);
    max_fd = server_fd;

    printf("select() server listening on port %d\n", PORT);

    while (1) {
        readfds = all_fds;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (client_fd < 0) { perror("accept"); continue; }

            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, ip, INET_ADDRSTRLEN);
            printf("New client: %s:%d on fd %d\n", ip, ntohs(address.sin_port), client_fd);

            for (i = 0; i < FD_SETSIZE; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = client_fd;
                    break;
                }
            }

            FD_SET(client_fd, &all_fds);
            if (client_fd > max_fd) max_fd = client_fd;
        }

        for (i = 0; i < FD_SETSIZE; i++) {
            int sd = client_fds[i];
            if (sd == -1) continue;

            if (FD_ISSET(sd, &readfds)) {
                ssize_t n = read(sd, buffer, sizeof(buffer) - 1);
                if (n <= 0) {
                    printf("fd %d disconnected\n", sd);
                    close(sd);
                    FD_CLR(sd, &all_fds);
                    client_fds[i] = -1;
                } else {
                    buffer[n] = '\0';
                    printf("fd %d: %s\n", sd, buffer);
                    write(sd, buffer, n);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
