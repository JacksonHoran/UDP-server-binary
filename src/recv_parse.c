// Simple UDP receiver that parses 'Add Order Short' (message type 'a') messages
// This is a small demo parser derived from `cfiles/raw/message_a.cpp`.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
    char messageType;
    uint16_t trackingNumber;
    uint64_t timestamp;
    uint64_t orderRefNumber;
    char marketSide;
    uint32_t optionId;
    uint16_t price;
    uint16_t volume;
} AddOrderShortMessage;

AddOrderShortMessage parseAddOrderShort(const unsigned char *msg) {
    AddOrderShortMessage a;

    a.messageType = msg[0];
    a.trackingNumber = (msg[1] << 8) | msg[2];
    a.timestamp = ((uint64_t)msg[3] << 40) |
                  ((uint64_t)msg[4] << 32) |
                  ((uint64_t)msg[5] << 24) |
                  ((uint64_t)msg[6] << 16) |
                  ((uint64_t)msg[7] << 8)  |
                  ((uint64_t)msg[8]);

    uint64_t tmp64;
    memcpy(&tmp64, msg + 9, sizeof(tmp64));
    a.orderRefNumber = __builtin_bswap64(tmp64);

    a.marketSide = msg[17];

    uint32_t tmp32;
    memcpy(&tmp32, msg + 18, sizeof(tmp32));
    a.optionId = ntohl(tmp32);

    a.price = (msg[22] << 8) | msg[23];
    a.volume = (msg[24] << 8) | msg[25];

    return a;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { perror("socket"); return EXIT_FAILURE; }

    // increase receive buffer to avoid drops under bursts
    int rcvbuf = 4 * 1024 * 1024; // 4MB
    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0) {
        perror("setsockopt SO_RCVBUF"); /* non-fatal */
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(s); return EXIT_FAILURE;
    }

    printf("recv_parse: listening on UDP port %d\n", port);

    unsigned char buf[2048];
    while (1) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        ssize_t n = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&peer, &plen);
        if (n < 0) {
            perror("recvfrom"); break;
        }

        if (n < 1) continue;

        // scan within the UDP packet for any complete messages starting with 0x61
        int found = 0;
        for (ssize_t off = 0; off < n; ++off) {
            if ((unsigned char)buf[off] != 0x61) continue; // not an 'a' message start
            if (off + 26 > n) break; // incomplete message in this packet
            AddOrderShortMessage m = parseAddOrderShort(buf + off);
            printf("parsed AddOrderShort from %s:%d | msg=%c tracking=%u option=%u price=%u vol=%u\n",
                   inet_ntoa(peer.sin_addr), ntohs(peer.sin_port), m.messageType,
                   m.trackingNumber, m.optionId, m.price, m.volume);
            found = 1;
        }
        if (!found) {
            printf("no parsable 'a' messages in packet from %s:%d (len=%zd)\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port), n);
        }
    }

    close(s);
    return 0;
}
