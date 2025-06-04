#include <csignal>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0
#define PORT "5566"
#define BACKLOG 5


void* get_in_addr(struct sockaddr* sockAddr)
{
    if(sockAddr->sa_family == AF_INET)
    {
        return &((struct sockaddr_in*)sockAddr)->sin_addr;
    }
    else 
    {
        return &((struct sockaddr_in6*)sockAddr)->sin6_addr;
    }
}
void sigchld_handler(int s)
{
    int savedErrno = errno;

    (void)s;

    while(waitpid(-1, NULL, WNOHANG) > 0); // -1 is looking for any existing child , null -> dont care about the child exit status, WNOHAND : dont block if child is not dead

    errno = savedErrno;
}
int main(int argc, char** argv)
{
    int sockfd, newfd;
    int yes = 1;
    
    struct addrinfo hint, *res, *p;
    struct sockaddr_storage theiraddr;
    struct sigaction sa;
    char ip_address[INET6_ADDRSTRLEN];
    errno = 0;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    int addrInfoStatus = getaddrinfo(NULL, PORT, &hint, &res);
    if(addrInfoStatus != 0)
    {
        fprintf(stderr, "Unlucky getaddrinfo was not initialized correctly..! getaddrinfo: %s\n", gai_strerror(addrInfoStatus));
        return 1;
    }
    for(p = res; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("Server: Socket");
            continue;
        }

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) // solves the tima wait thing when binding to the same port after killing the server
        {
            perror("setsockopt");
            exit(1);
        }
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("Server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(res);

    if(p == NULL)
    {
        fprintf(stderr, "Server Failed To Bind...!\n");
        exit(1);
    }
    if(listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");
    while(TRUE)
    {
        socklen_t inaddr_size = sizeof(theiraddr);
        if((newfd = accept(sockfd, (struct sockaddr*)&theiraddr, &inaddr_size)) == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(theiraddr.ss_family, get_in_addr((struct sockaddr*)&theiraddr), ip_address, sizeof(ip_address));
        printf("Server Got Connection from %s client\n", ip_address);


        if(!fork()) //fork returns 0 if its child process if(!0 == true) means it's child
        {
            const char* msg = "Hello Client\n";
            close(sockfd);
            if(send(newfd, msg, strlen(msg), 0) == -1)
                perror("Server: Send");

            close(newfd);
            exit(0);
        }
        close(newfd); // close the new socket for with the client in the parent process
    }

   
    return 0;
}
