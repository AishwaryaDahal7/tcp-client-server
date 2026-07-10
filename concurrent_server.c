#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_fd, struct sockaddr_in *client_addr) {
    char buffer[BUFFER_SIZE] = {0};
    char client_ip[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("[PID %d] Client connected: %s:%d\n",
           getpid(), client_ip, ntohs(client_addr->sin_port));

    ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("[PID %d] Received: %s\n", getpid(), buffer);
        char *reply = "Hello from concurrent server!";
        write(client_fd, reply, strlen(reply));
    }

    close(client_fd);
    printf("[PID %d] Client %s disconnected\n", getpid(), client_ip);
    exit(0);
}

void reap_zombies(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    signal(SIGCHLD, reap_zombies);
    signal(SIGINT, SIG_IGN);

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

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Concurrent TCP server listening on port %d (PID %d)\n",
           PORT, getpid());

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            close(server_fd);
            handle_client(client_fd, &address);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
