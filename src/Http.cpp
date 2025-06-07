#include "Defines.h"
#include <cstdio>
#include <string.h>
#include <sys/socket.h>
#include "Http.h"


// maybe will change this in future to use less memcmp 
HTTP_METHOD GetHttpMethodFromStr(const char *methodStr)
{
    size_t len = strlen(methodStr); // what if methodstr was GETTER the memcmp will return true cuz first 3 letter is equal to GET !!!!
    if(len == 3 && memcmp(methodStr, "GET", 3) == 0)
    {
        return HTTP_METHOD::GET;
    }
    else if(len == 4 && memcmp(methodStr, "POST", 4) == 0)
    {
        return HTTP_METHOD::POST;
    }
    else if(len == 3 && memcmp(methodStr, "PUT", 3) == 0)
    {
        return HTTP_METHOD::PUT;
    }
    else if(len == 6 && memcmp(methodStr, "DELETE", 6) == 0)
    {
        return HTTP_METHOD::DELETE;
    }
    return HTTP_METHOD::NONE;
}

HTTP_READ_STATUS ReadHttpRequest(int sockfd, char* outRequestBuffer, int &outTotalBytesRead)
{

    char currentChunk[1024];

    while(TRUE)
    {
        ssize_t bytesReceived = recv(sockfd, currentChunk, sizeof(currentChunk), 0);

        if(bytesReceived == 0)
        {
            return HTTP_READ_STATUS::CLIENT_CLOSED_CONNECTION;
        }
        if(outTotalBytesRead + bytesReceived >= MAX_HTTP_REQUEST_SIZE - 1) // check for overflow
        {
            return HTTP_READ_STATUS::BUFFER_FULL;                                                                                
        }
        if(bytesReceived == -1)
        {
            perror("recv");
            return HTTP_READ_STATUS::ERROR;
        }
        memcpy(outRequestBuffer + outTotalBytesRead, currentChunk, bytesReceived);

        outTotalBytesRead += bytesReceived;
        outRequestBuffer[outTotalBytesRead] = '\0'; // adding the null term after every line read from the request
        if(strstr(outRequestBuffer, "\r\n\r\n"))
        {
            return HTTP_READ_STATUS::SUCCESS;
        }
    }
}
bool ParseHttpRequest(HttpRequest &request, char* requestBuffer)
{
    if(strstr(requestBuffer, "\r\n\r\n"))
    {
        char httpMethod[8];
        char httpPath[2048];
        char httpVersion[16];
        sscanf(requestBuffer, "%7s %2047s %15s", httpMethod, httpPath, httpVersion); // change it to manual parsing in future to prevent potential bugs
        request.RequestLine.Method = GetHttpMethodFromStr(httpMethod);
        printf("httpMethod: %s, httpPath: %s, httpVersion: %s\n", httpMethod, httpPath, httpVersion);
        return TRUE;
    }
    return FALSE;
}
