#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int server_fd;

void handle_sigint(int sig) {
    printf("\n[SIGINT] Shutting down server...\n");
    close(server_fd);
    exit(0);
}

void handle_sigterm(int sig) {
    printf("\n[SIGTERM] Terminating server...\n");
    close(server_fd);
    exit(0);
}

void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    printf("[SIGCHLD] Reaped zombie child\n");
}

void handle_sigpipe(int sig) {
    printf("[SIGPIPE] Broken pipe - client disconnected unexpectedly\n");
}

void handle_sighup(int sig) {
    printf("[SIGHUP] Hangup signal received (config reload placeholder)\n");
}

void handle_client(int client_fd, struct sockaddr_in *client_addr) {
    char buffer[BUFFER_SIZE] = {0};
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, ip, INET_ADDRSTRLEN);
    printf("[Child %d] Serving client %s:%d\n", getpid(), ip, ntohs(client_addr->sin_port));

    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("[Child %d] Received: %s\n", getpid(), buffer);

        if (write(client_fd, buffer, n) < 0 && errno == EPIPE)
            printf("[Child %d] Write failed - client gone\n", getpid());
    }

    close(client_fd);
    printf("[Child %d] Done\n", getpid());
    exit(0);
}

int main() {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigterm);
    signal(SIGCHLD, handle_sigchld);
    signal(SIGPIPE, handle_sigpipe);
    signal(SIGHUP, handle_sighup);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Signal handling server on port %d (PID %d)\n", PORT, getpid());
    printf("  SIGINT  - graceful shutdown (Ctrl+C)\n");
    printf("  SIGTERM - terminate\n");
    printf("  SIGCHLD - reap zombies\n");
    printf("  SIGPIPE - broken pipe\n");
    printf("  SIGHUP  - hangup / reload\n");

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_fd < 0) continue;

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_client(client_fd, &address);
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
