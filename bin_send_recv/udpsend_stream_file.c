// NAME: udpsend_stream_file.c

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CHUNK_SIZE 1024 // bytes per packet

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 6) {
        printf("Usage: %s <peer_ip> <peer_port> <filename> [chunk_size] [delay_us]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *peer_ip = argv[1];
    int peer_port = atoi(argv[2]);
    const char *filename = argv[3];
    int chunk_size = CHUNK_SIZE;
    int delay_us = 10000; // default 10ms throttle
    if (argc >= 5) {
        int cs = atoi(argv[4]);
        if (cs > 0) chunk_size = cs;
    }
    if (argc == 6) {
        int d = atoi(argv[5]);
        if (d >= 0) delay_us = d;
    }

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

    char *buffer = malloc(chunk_size);
    if (!buffer) {
        perror("malloc");
        close(udp_socket);
        return EXIT_FAILURE;
    }
    size_t bytes_read;
    size_t total_sent = 0;
    int packet_count = 0;
    while ((bytes_read = fread(buffer, 1, chunk_size, file)) > 0) {
        ssize_t sent = sendto(udp_socket, buffer, bytes_read, 0,
                              (struct sockaddr *)&peer_addr, sizeof(peer_addr));
        if (sent < 0) {
            perror("Send failed");
            break;
        }

        total_sent += sent;
        packet_count++;

        printf("Sent packet #%d (%zd bytes)\n", packet_count, sent);

        if (delay_us > 0) usleep(delay_us);
    }

    printf("Finished sending %zu bytes in %d packets.\n", total_sent, packet_count);

    fclose(file);
    close(udp_socket);
    return EXIT_SUCCESS;
}
