#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int server_fd;

void sigio_handler(int sig) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd < 0) return;

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
    printf("Client connected: %s:%d\n", ip, ntohs(client_addr.sin_port));

    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("Received: %s\n", buffer);
        write(client_fd, buffer, n);
    }

    close(client_fd);
    printf("Client disconnected\n");
}

int main() {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    signal(SIGIO, sigio_handler);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    fcntl(server_fd, F_SETOWN, getpid());
    int flags = fcntl(server_fd, F_GETFL);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK | O_ASYNC);

    printf("Signal-driven I/O server on port %d (PID %d)\n", PORT, getpid());
    printf("Waiting for SIGIO...\n");

    while (1) pause();

    close(server_fd);
    return 0;
}
