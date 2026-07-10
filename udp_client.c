#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    socklen_t addrlen = sizeof(serv_addr);
    char buffer[BUFFER_SIZE] = {0};
    const char *message;

    if (argc > 1)
        message = argv[1];
    else
        message = "Hello UDP Server!";

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Sending to server: %s\n", message);

    sendto(sock, message, strlen(message), 0,
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    ssize_t bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr *)&serv_addr, &addrlen);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server reply: %s\n", buffer);
    }

    close(sock);
    return 0;
}
