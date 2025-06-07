// #include <csignal>
// #include <cstdlib>
// #include <stdlib.h>
// #include <unistd.h>
// #include <stddef.h>
// #include <netinet/in.h>
// #include <stdio.h>
// #include <string.h>
// #include <stddef.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <errno.h>
// #include <sys/wait.h>
// #include <signal.h>
//
//
// #define TRUE 1
// #define FALSE 0
// #define PORT "5566"
//
// void* get_in_addr(struct sockaddr* sa)
// {
//     if(sa->sa_family == AF_INET)
//     {
//         return &((struct sockaddr_in*)sa)->sin_addr;
//     }
//     else 
//     {
//         return &((struct sockaddr_in6*)sa)->sin6_addr;
//     }
// }
// int main(int argc, char** argv)
// {
//     int sockfd, addr_info_status;
//     struct addrinfo hints, *res, *p;
//     char ip_address[INET6_ADDRSTRLEN];
//     if(argc != 3)
//     {
//         fprintf(stderr, "Usage: client host port\n");
//         exit(1);
//     }
//     memset(&hints, 0, sizeof(hints));
//     hints.ai_family = AF_UNSPEC;
//     hints.ai_socktype = SOCK_STREAM;
//     char* host = argv[1];
//     char* port = argv[2];
//     addr_info_status = getaddrinfo(host, port, &hints, &res);
//
//     if(addr_info_status != 0)
//     {
//         fprintf(stderr, "addrinfo : %s\n", gai_strerror(addr_info_status));
//         exit(1);
//     }
//     for(p = res; p != NULL; p = p->ai_next)
//     {
//         sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
//         if(sockfd == -1)
//         {
//             perror("client socket");
//             continue;
//         }
//         inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), ip_address, sizeof(ip_address));
//         printf("Client %s Attemping to connect to host %s on port %s\n", ip_address, host, port);
//         if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
//         {
//             perror("client connect");
//             close(sockfd);
//             continue;
//         }
//         break;
//     }
//     if(p == NULL)
//     {
//         fprintf(stderr, "Client: Failed To Connect to %s\n", host);
//         return 2;
//     }
//     inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), ip_address, sizeof(ip_address));
//     printf("Client %s successfully connected to host %s on port %s\n", ip_address, host, port);
//     freeaddrinfo(res);
//
//     char buffer[100];
//     size_t read_buffer_bytes = recv(sockfd, buffer, sizeof(buffer), 0);
//     printf("message is : %s\n", buffer);
//
//     close(sockfd);
// }
