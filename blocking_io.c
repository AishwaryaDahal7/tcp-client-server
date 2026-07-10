#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    printf("Blocking I/O server on port %d\n", PORT);

    client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address.sin_addr, ip, INET_ADDRSTRLEN);
    printf("Client connected: %s:%d\n", ip, ntohs(address.sin_port));

    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    buffer[n] = '\0';
    printf("Received: %s\n", buffer);
    write(client_fd, buffer, n);

    close(client_fd);
    close(server_fd);
    return 0;
}
