#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>

#define BACKLOG 1       
#define BUF_SIZE 32768       // 32KB buffer size
#define TEST_SECS   30    
#define WINDOW_SECS 2 

int main(void) {
    const char *port = "0"; // let the OS choose the port
    struct addrinfo hints, *res, *p;
    int sockfd, new_fd ;  
    // generic address buffer as we don't know yet if the address is IPv4 or IPv6
    struct sockaddr_storage client_addr; 
    socklen_t addr_len;
    char buf[BUF_SIZE];

    
    memset(&hints, 0, sizeof hints); // initialization with zeros 
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;        // accept connections on any network interface

    // Resolve local address and port
    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return EXIT_FAILURE;
    }

    
    //Loop through each candidate address in the list returned by getaddrinfo() , when first 
    //suitable candidate found then create socket and bind on a port
    for (p = res; p; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;
        // Try to bind on the port that the OS choose
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;  // success
        close(sockfd);
    }
    
    // we don't need the linked list anymore so all the nodes 
    freeaddrinfo(res);
    
    // after the loop above if we failed to bind then we reached the end of the list 
    // and the p would be NULL so we failed to bind on every candidate address
    if (!p) {
        fprintf(stderr, "server: failed to bind\n");
        return EXIT_FAILURE;
    }
    

    struct sockaddr_storage local_addr;
    socklen_t local_len = sizeof local_addr;
    
    // Ask kernel to fill local_addr with the local address and port that sockfd is bound to
    if (getsockname(sockfd, (struct sockaddr *)&local_addr, &local_len) == -1) {
        perror("getsockname");
        close(sockfd);
        return EXIT_FAILURE;
    }

    uint16_t assigned_port;
    if (local_addr.ss_family == AF_INET) {
        assigned_port = ntohs(((struct sockaddr_in *)&local_addr)->sin_port); // we cast into sockaddr_in * to access sin_port
    } else {
        assigned_port = ntohs(((struct sockaddr_in6 *)&local_addr)->sin6_port); //use ntohs() to print the port in the machine's byte order so that client connects correctly
    }
    
   // Extract the IP string to give to client to know if its ipv4 or pv6
   char ipstr[INET6_ADDRSTRLEN];
   if (local_addr.ss_family == AF_INET) {
       struct sockaddr_in *s4 = (struct sockaddr_in *)&local_addr;
       inet_ntop(AF_INET, &s4->sin_addr, ipstr, sizeof ipstr);
   } else {
       struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)&local_addr;
       inet_ntop(AF_INET6, &s6->sin6_addr, ipstr, sizeof ipstr);
   }


    // Listen
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        return EXIT_FAILURE;
    }
    
    printf("Listening on IP %s: Port %u\n", ipstr, assigned_port);
    
   while(1){ //keep alive the server
      // Accept one client
      addr_len = sizeof client_addr;
      new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len); //blocks until a client connects
      if (new_fd == -1) {
          perror("accept");
          return EXIT_FAILURE;
    }
    char addr_str[INET6_ADDRSTRLEN];//buffer large enough to hold the text representation of an IPv6 address 
    
    void *addr;
    if (client_addr.ss_family == AF_INET) {  
        // It’s IPv4: cast to sockaddr_in and take the sin_addr field
        struct sockaddr_in *sa4 = (struct sockaddr_in *)&client_addr;
        addr = &sa4->sin_addr;
    } else {
        // It’s IPv6: cast to sockaddr_in6 and take the sin6_addr field
        struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&client_addr;
        addr = &sa6->sin6_addr;
}
    // turn the bianry IP into readable string for printing
    inet_ntop(client_addr.ss_family, addr, addr_str, sizeof addr_str);
    printf("Connection from %s\n", addr_str);
    
 

    // Receive data and calculate throughput
    time_t start_time = time(NULL);
    time_t window_start = start_time;
    size_t window_bytes = 0;
    size_t total_bytes  = 0;
    time_t now;


    while ((now = time(NULL)) - start_time < TEST_SECS) {
    ssize_t n = recv(new_fd, buf, BUF_SIZE, 0);
    if (n <= 0) break;
        window_bytes += n;
        total_bytes  += n;
        
        if ((now - window_start) >= WINDOW_SECS) {
            double seconds = (double)(now - window_start);
            double mbps = (window_bytes * 8) / (seconds * 1e6);
            printf("[%2ld s] %.2f Mbps \n",
                   now - start_time,
                   mbps);
            window_start = now;
            window_bytes = 0;
        }
    }

    // Flush the final window if data was received but not yet printed
     if (window_bytes > 0) {
       double seconds = (double)(now - window_start);
        if (seconds > 0) {
           double mbps = (window_bytes * 8) / (seconds * 1e6);
           printf("[%2ld s] %.2f Mbps \n", now - start_time, mbps);
         }
       }


    double avg_mbps = (total_bytes * 8) / (TEST_SECS * 1e6);
    printf("[DONE] Average received throughput : %.2f Mbps \n",avg_mbps);
           
    close(new_fd);
}    
    close(sockfd);

    return EXIT_SUCCESS;
}
