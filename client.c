#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

#define BUF_SIZE 32768       // 32KB buffer size
#define WINDOW_SECS     2
#define TEST_SECS      30

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    //argv[0] is programm's name "./client"
    const char *host = argv[1];
    const char *port = argv[2];
    struct addrinfo hints, *res, *p;
    int sockfd;
    char buf[BUF_SIZE];
    
    // fill buffer with data
    memset(buf, 'A', BUF_SIZE);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Resolve server address
    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return EXIT_FAILURE;
    }

    // Connect to the first valid result
    for (p = res; p; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;  // success
        close(sockfd);
    }
    if (!p) {
        fprintf(stderr, "client: failed to connect\n");
        return EXIT_FAILURE;
    }
    freeaddrinfo(res);

    printf("Connected to %s:%s\n", host, port);
    
    //transmits data
    size_t total_bytes  = 0;
    size_t window_bytes = 0;

    time_t start_time   = time(NULL);
    time_t window_start = start_time;

    while (time(NULL) - start_time < TEST_SECS) {

        ssize_t n;
        do {                             
            n = send(sockfd, buf, BUF_SIZE, 0);
        } while (n == -1);

        if (n == -1) {                   
            perror("send");
            break;
        }

        window_bytes += (size_t)n;
        total_bytes  += (size_t)n;

        time_t now = time(NULL);
        if (now - window_start >= WINDOW_SECS) {
            double mbps = (window_bytes * 8.0) / ((now - window_start) * 1e6);
                          
            printf("[%2ld s] %.2f Mbps\n",now - start_time, mbps);
            
            window_start = now;
            window_bytes = 0;
        }
    }

    double avg_mbps = (total_bytes * 8.0) / (TEST_SECS  * 1e6);

    printf("[DONE] Average send throughput: %.2f Mbps\n",avg_mbps);

    close(sockfd);
    return EXIT_SUCCESS;
   
}
