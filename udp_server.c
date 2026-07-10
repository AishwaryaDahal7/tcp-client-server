#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in address, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE] = {0};

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("UDP Server listening on port %d...\n", PORT);

    ssize_t bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (struct sockaddr *)&client_addr, &addrlen);
    if (bytes < 0) {
        perror("recvfrom failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    buffer[bytes] = '\0';

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Received from %s:%d: %s\n", client_ip, ntohs(client_addr.sin_port), buffer);

    char *reply = "Hello from UDP server!";
    sendto(sock, reply, strlen(reply), 0,
           (struct sockaddr *)&client_addr, addrlen);

    close(sock);
    return 0;
}
