#include "Defines.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include "Http.h"



void* GetInAddress(struct sockaddr* sockAddr)
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
void SignalChildHandler(int s)
{
    int savedErrno = errno;

    (void)s;

    while(waitpid(-1, NULL, WNOHANG) > 0); // -1 is looking for any existing child , null -> dont care about the child exit status, WNOHAND : dont block if child is not dead

    errno = savedErrno;
}
int main(int argc, char** argv)
{
    if(argc != 2)
    {
        fprintf(stderr, "Usage : server port\n");
        exit(1);
    }
    char* port = argv[1];
    int sockfd, newfd;
    int yes = 1;
    
    struct addrinfo hint, *res, *p;
    struct sockaddr_storage theirAddress;
    struct sigaction signalAction;
    char ipAddress[INET6_ADDRSTRLEN];
    errno = 0;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    int addrInfoStatus = getaddrinfo(NULL, port, &hint, &res);
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
    signalAction.sa_handler = SignalChildHandler;
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &signalAction, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    printf("Server: waiting for connections...\n");
    while(TRUE)
    {
        socklen_t inaddrSize = sizeof(theirAddress);
        if((newfd = accept(sockfd, (struct sockaddr*)&theirAddress, &inaddrSize)) == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(theirAddress.ss_family, GetInAddress((struct sockaddr*)&theirAddress), ipAddress, sizeof(ipAddress));
        printf("Server Got Connection from %s client\n", ipAddress);


        if(!fork()) //fork returns 0 if its child process if(!0 == true) means it's child
        {
            HttpRequest httpRequest;
            char requestBuffer[MAX_HTTP_REQUEST_SIZE];
            int totalBytesRead = 0;

            HTTP_READ_STATUS httpReadStatus = ReadHttpRequest(newfd, requestBuffer, totalBytesRead);
            

            switch (httpReadStatus) 
            {
                case HTTP_READ_STATUS::SUCCESS:
                    ParseHttpRequest(httpRequest, requestBuffer);
                    break;
                case HTTP_READ_STATUS::ERROR:
                    fprintf(stderr, "Failed to read request from client.\n");
                    break;
                case HTTP_READ_STATUS::CLIENT_CLOSED_CONNECTION:
                    printf("Client closed connection before sending a full request.\n"); 
                    break;
                case HTTP_READ_STATUS::BUFFER_FULL:
                  break;
            }

            HttpResponse response = HandleHttpRequest(httpRequest);
            size_t headersSize = 
            snprintf(NULL, 0, "HTTP/1.1 %d \r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %lu\r\n"
                "\r\n"
                ,(uint16_t)response.Status, response.ContentSize);
            size_t totalResponseSize = response.ContentSize + headersSize;
            char* responseToSend = (char*)malloc(totalResponseSize);
            int headersWritten = sprintf(responseToSend, "HTTP/1.1 %d \r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %lu\r\n"
                "\r\n"
                ,(uint16_t)response.Status, response.ContentSize);
            memcpy(responseToSend + headersWritten, response.HtmlFile, response.ContentSize);

            close(sockfd);
            if(send(newfd, responseToSend, totalResponseSize, 0) == -1)
                perror("Server: Send");

            close(newfd);
            free(response.HtmlFile);
            exit(0);
        }
        close(newfd); 
    }

    
   
    return 0;
}
