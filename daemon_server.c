#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define LOG_FILE "/tmp/daemon_server.log"

int server_fd;
FILE *log_fp;

void log_message(const char *msg) {
    fprintf(log_fp, "[PID %d] %s\n", getpid(), msg);
    fflush(log_fp);
}

void cleanup(int sig) {
    log_message("Shutting down daemon");
    close(server_fd);
    fclose(log_fp);
    exit(0);
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main() {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    daemonize();

    log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) exit(EXIT_FAILURE);

    log_message("Daemon started");

    signal(SIGTERM, cleanup);
    signal(SIGINT, cleanup);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { log_message("socket failed"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_message("bind failed"); exit(1);
    }
    if (listen(server_fd, 5) < 0) {
        log_message("listen failed"); exit(1);
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "Listening on port %d", PORT);
    log_message(msg);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_fd < 0) continue;

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            char buffer[BUFFER_SIZE] = {0};
            ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                fprintf(log_fp, "Received: %s\n", buffer);
                fflush(log_fp);
                write(client_fd, buffer, n);
            }
            close(client_fd);
            exit(0);
        }
        close(client_fd);
    }

    cleanup(0);
    return 0;
}
