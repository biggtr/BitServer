#include "Defines.h"
#include <cstdio>
#include <string.h>
#include <strings.h>
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
// TODO: change it to manual parsing to handle spaces and solve other bugs 
bool ParseHttpRequest(HttpRequest &request, char* requestBuffer)
{
    char* endOfHeaders = strstr(requestBuffer, "\r\n\r\n");

    if(!endOfHeaders)
    {
        return FALSE; // incomplete request
    }
    char* firstLineEnd = strstr(requestBuffer, "\r\n"); // get the first line to correctly parse the request line
    if(!firstLineEnd)
    {
        return FALSE; //didn't find the first request line (bad request)
    }
    char savedfirstLineChar = *firstLineEnd;
    *firstLineEnd = '\0'; // replace \r with \0 to tell sscanf to stop reading when it reachs \r in the first line \r\n

    char httpMethod[8];
    char httpPath[2048];
    char httpVersion[16];
    int requestLineParsed = sscanf(requestBuffer, "%7s %2047s %15s", httpMethod, httpPath, httpVersion); 
    if(requestLineParsed != 3)
    {
        return FALSE; // didn't parse the request line correctly maybe user sent some malicious code  
    }
    *firstLineEnd = savedfirstLineChar;
    request.RequestLine.Method = GetHttpMethodFromStr(httpMethod);
    strcpy(request.RequestLine.URL, httpPath);
    strcpy(request.RequestLine.HttpVersion, httpVersion);
    printf("httpMethod: %s, httpPath: %s, httpVersion: %s\n", httpMethod, httpPath, httpVersion);

    // TODO: parse the rest of the request headers
    char* headerStart = firstLineEnd + 2; // skipping the firstLineEnd \r\n to get to the first header;
    char* endHost = strchr(headerStart, '\r');
    int len = endHost - headerStart;
    char host[256];
    host[len] = '\0';
    strncpy(host, headerStart, len);

    printf("%s\n", host);
    return TRUE;
}
