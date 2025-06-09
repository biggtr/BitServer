#include "Defines.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
    uint8_t headerNumber = 0;
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
    request.ReqLine.Method = GetHttpMethodFromStr(httpMethod);
    strcpy(request.ReqLine.URL, httpPath);
    strcpy(request.ReqLine.HttpVersion, httpVersion);
    printf("httpMethod: %s, httpPath: %s, httpVersion: %s\n", httpMethod, httpPath, httpVersion);

    // TODO: parse the rest of the request headers
    char* headerStart = firstLineEnd + 2; // skipping the firstLineEnd \r\n to get to the first header;
    while(headerStart != endOfHeaders && headerStart < endOfHeaders) 
    {
        char* headerEnd = strchr(headerStart, '\r');
        char* headerName = strchr(headerStart, ' ') ;
        int headerNameLen = headerName - headerStart;
        char* headerValueStart = headerName + 1;
        int headerValueLen = headerEnd - headerValueStart;
        strncpy(request.Headers[headerNumber].Name, headerStart, headerNameLen);
        strncpy(request.Headers[headerNumber].Value, headerValueStart, headerValueLen);
        request.Headers[headerNumber].Name[headerNameLen] = '\0';
        request.Headers[headerNumber].Value[headerValueLen] = '\0';
        printf("%s %s\n", request.Headers[headerNumber].Name, request.Headers[headerNumber].Value);
        headerNumber++;
        headerStart = headerEnd + 2;
        if(headerStart >= endOfHeaders)
        {
            return FALSE;
        }
    }
    return TRUE;
}

// TODO: Change from ffamily(stdio) to syscalls for performance 
HttpResponse HandleHttpRequest(HttpRequest& request)
{
    // TODO: remove this logic from here and handle it inside GET
    HttpResponse response;
    const char* fileName = strcmp(request.ReqLine.URL, "/") == 0 ? "index.html" : request.ReqLine.URL;
    char finalFileName[100];
    snprintf(finalFileName, sizeof(finalFileName), "../html/%s", fileName);
    FILE* htmlFile = fopen(finalFileName, "r");
    if(htmlFile == NULL)
    {
        perror("fopen");
    }
    if(fseek(htmlFile, 0, SEEK_END) == -1)
    {
        perror("fseek:");
    }
    long htmlBytesNum = ftell(htmlFile);
    printf("htmlBytesNum: %lu", htmlBytesNum);
    response.HtmlFile = (char*)malloc(htmlBytesNum + 1); // 1 for null term
    fseek(htmlFile, 0, SEEK_SET);
    
    size_t bytesRead = fread(response.HtmlFile, sizeof(char), htmlBytesNum, htmlFile);
    if(bytesRead != htmlBytesNum)
    {
        fprintf(stderr, "Error reading file\n");
        free(response.HtmlFile); 
        fclose(htmlFile);
    }
    response.HtmlFile[htmlBytesNum] = '\0';
    response.Status = HTTP_STATUS::OK;
    response.ContentSize = htmlBytesNum;
    printf("%s\n", response.HtmlFile);

    switch (request.ReqLine.Method) 
    {
        case HTTP_METHOD::GET:
        case HTTP_METHOD::POST:
        case HTTP_METHOD::PUT:
        case HTTP_METHOD::DELETE:
        case HTTP_METHOD::NONE:
          break;
    }
    return response;

}

