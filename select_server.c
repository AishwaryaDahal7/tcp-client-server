#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main() {
    int server_fd, client_fds[MAX_CLIENTS], max_fd, activity, i;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    fd_set readfds;
    char buffer[BUFFER_SIZE] = {0};

    for (i = 0; i < MAX_CLIENTS; i++)
        client_fds[i] = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Select-based TCP server listening on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_fds[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_fd)
                max_fd = sd;
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select error");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int new_client = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (new_client < 0) {
                perror("accept failed");
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
            printf("New client: %s:%d fd=%d\n",
                   client_ip, ntohs(address.sin_port), new_client);

            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = new_client;
                    break;
                }
            }

            if (i == MAX_CLIENTS)
                printf("Max clients reached, rejecting %d\n", new_client);
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_fds[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                ssize_t bytes = read(sd, buffer, sizeof(buffer) - 1);
                if (bytes <= 0) {
                    printf("Client fd=%d disconnected\n", sd);
                    close(sd);
                    client_fds[i] = 0;
                } else {
                    buffer[bytes] = '\0';
                    printf("fd=%d says: %s\n", sd, buffer);
                    char *reply = "Hello from select server!";
                    write(sd, reply, strlen(reply));
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
