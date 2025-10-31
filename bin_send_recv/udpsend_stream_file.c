// NAME: udpsend_stream_file.c

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CHUNK_SIZE 1024 // bytes per packet

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <peer_ip> <peer_port> <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *peer_ip = argv[1];
    int peer_port = atoi(argv[2]);
    const char *filename = argv[3];

    struct sockaddr_in peer_addr = {.sin_family = AF_INET, .sin_port = htons(peer_port)};
    if (inet_pton(AF_INET, peer_ip, &(peer_addr.sin_addr)) <= 0) {
        perror("Invalid IP address");
        return EXIT_FAILURE;
    }

    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        close(udp_socket);
        return EXIT_FAILURE;
    }

    printf("Streaming file \"%s\" to %s:%d...\n", filename, peer_ip, peer_port);

    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    size_t total_sent = 0;
    int packet_count = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        ssize_t sent = sendto(udp_socket, buffer, bytes_read, 0,
                              (struct sockaddr *)&peer_addr, sizeof(peer_addr));
        if (sent < 0) {
            perror("Send failed");
            break;
        }

        total_sent += sent;
        packet_count++;

        // Optional: throttle a little to avoid overwhelming receiver
        usleep(1); // 10ms delay
    }

    printf("Finished sending %zu bytes in %d packets.\n", total_sent, packet_count);

    fclose(file);
    close(udp_socket);
    return EXIT_SUCCESS;
}
