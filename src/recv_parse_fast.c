// Fast UDP receiver that parses 'Add Order Short' messages and batches tracking numbers
// Writes tracking numbers to received_fast.txt in order of arrival.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

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

static volatile sig_atomic_t stop_requested = 0;
static void handle_sigint(int s) { (void)s; stop_requested = 1; }

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) { perror("socket"); return EXIT_FAILURE; }

    int rcvbuf = 8 * 1024 * 1024; // 8MB receive buffer
    setsockopt(s, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(s); return EXIT_FAILURE;
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    fprintf(stderr, "recv_parse_fast: listening on UDP port %d\n", port);

    unsigned char buf[2048];

    // batching
    size_t cap = 16384;
    size_t len = 0;
    unsigned int *batch = malloc(cap * sizeof(unsigned int));
    if (!batch) { perror("malloc"); close(s); return EXIT_FAILURE; }
    FILE *out = fopen("received_fast.txt", "w");
    if (!out) { perror("fopen"); free(batch); close(s); return EXIT_FAILURE; }

    const size_t FLUSH = 4096;

    while (!stop_requested) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        ssize_t n = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&peer, &plen);
        if (n <= 0) continue;
        for (ssize_t off = 0; off + 26 <= n; ++off) {
            if (buf[off] != 0x61) continue;
            AddOrderShortMessage m = parseAddOrderShort(buf + off);
            if (len >= cap) {
                cap *= 2;
                unsigned int *nb = realloc(batch, cap * sizeof(unsigned int));
                if (!nb) { perror("realloc"); stop_requested = 1; break; }
                batch = nb;
            }
            batch[len++] = m.trackingNumber;
            if (len >= FLUSH) {
                for (size_t i = 0; i < len; ++i) fprintf(out, "%u\n", batch[i]);
                fflush(out);
                len = 0;
            }
        }
    }

    // flush remaining
    for (size_t i = 0; i < len; ++i) fprintf(out, "%u\n", batch[i]);
    fclose(out);
    free(batch);
    close(s);
    fprintf(stderr, "recv_parse_fast: stopped, wrote received_fast.txt\n");
    return 0;
}
