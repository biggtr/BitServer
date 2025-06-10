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
    TOLOWER(httpPath);
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
        // printf("%s %s\n", request.Headers[headerNumber].Name, request.Headers[headerNumber].Value);
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
    HttpResponse response{};

    switch (request.ReqLine.Method) 
    {
        case HTTP_METHOD::GET:
            response = HandleGetRequest(request.ReqLine.URL);
            break;
        case HTTP_METHOD::POST:
        case HTTP_METHOD::PUT:
        case HTTP_METHOD::DELETE:
        case HTTP_METHOD::NONE:
          break;
    }
    return response;

}

HttpResponse HandleGetRequest(const char* url)
{

    HttpResponse response;
    response.ContentType = "text/html";
    // Prevent path traversal 
    if(strstr(url, "..") != NULL || strchr(url, '~') != NULL)
    {
        response.Status = HTTP_STATUS::FORBIDDEN;
        response.Content = strdup("<h1>403 Forbidden!");
        response.ContentSize = strlen(response.Content);
        return response;
    }
    const char* fileName = strcmp(url, "/") == 0 ? "index" : url + 1; // skip the first / in url
    char finalFileName[100];
    int pathLen = snprintf(finalFileName, sizeof(finalFileName), "../html/%s.html", fileName);
    if(pathLen >= sizeof(finalFileName))
    {
        printf("sizeof final name : %lu strlen : %lu", sizeof(finalFileName), strlen(finalFileName));
        response.Status = HTTP_STATUS::URI_TOO_LONG;
        response.Content = strdup("<h1>414 URI_TOO_LONG..!</h1>");
        response.ContentSize = strlen(response.Content);
        return response;
    }
    TOLOWER(finalFileName);
    FILE* htmlFile = fopen(finalFileName, "r");
    if(htmlFile == NULL)
    {
        perror("fopen");
        response.Status = HTTP_STATUS::NOT_FOUND;
        response.Content = strdup("<h1>404 NOT FOUND..!</h1>");
        response.ContentSize = strlen(response.Content);
        return response;
    }
    if(fseek(htmlFile, 0, SEEK_END) == -1)
    {
        fclose(htmlFile);
        perror("fseek:");
        response.Status = HTTP_STATUS::INTERNAL_SERVER_ERROR;
        response.Content = strdup("<h1>500 INTERNAL SERVER ERROR</h1>");
        response.ContentSize = strlen(response.Content);
        return response;
    }
    long fileSize = ftell(htmlFile);
    if(fileSize == -1 || fileSize >= 10 * 1024 * 1024) // limit the file to 10MB
    {
        fclose(htmlFile);
        perror("ftell:");
        response.Status = HTTP_STATUS::INTERNAL_SERVER_ERROR;
        response.Content = strdup("<h1>500 INTERNAL SERVER ERROR</h1>");
        response.ContentSize = strlen(response.Content);
        return response;
    }
    fseek(htmlFile, 0, SEEK_SET); // go back to the beginning of the file
                                  
    response.Content = (char*)malloc(fileSize + 1); // 1 for null term
    if(!response.Content) 
    {
        fclose(htmlFile);
        response.Status = HTTP_STATUS::INTERNAL_SERVER_ERROR;
        response.Content = strdup("<h1>500 INTERNAL SERVER ERROR</h1>");
        response.ContentSize = strlen(response.Content);
        return response;
    }
    size_t bytesRead = fread(response.Content, sizeof(char), fileSize, htmlFile);
    fclose(htmlFile);
    if(bytesRead != fileSize)
    {
        free(response.Content);
        response.Status = HTTP_STATUS::INTERNAL_SERVER_ERROR;
        response.Content = strdup("<h1>500 INTERNAL SERVER ERROR</h1>");
        response.ContentSize = strlen(response.Content);
        return response;
    }
    response.Content[fileSize] = '\0';
    response.ContentSize = fileSize;
    response.Status = HTTP_STATUS::OK;
    return response;
}
void SendHttpResponse(int sockfd, HttpResponse& response)
{
    char headers[1024];
    int headersLen = sprintf(headers, 
            "HTTP/1.1 %d %s \r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lu\r\n"
            "\r\n",
            (uint16_t)response.Status,
            GetStatusText(response.Status),
            response.ContentType,
            response.ContentSize);
    if(headersLen > sizeof(headers))
    {
        perror("Server: Header allocation failed");
        return;
    }
    if(send(sockfd, headers, headersLen, MSG_MORE) == -1)
    {
        perror("Server: Send");
        return;
    }

    if(response.Content && response.ContentSize > 0)
    {
        if(send(sockfd, response.Content, response.ContentSize, 0) == -1)
        {
            perror("Server: Send");
        }
    }
}

const char* GetStatusText(HTTP_STATUS status)
{
    switch(status) 
    {

        case HTTP_STATUS::OK: return "OK";
        case HTTP_STATUS::CREATED: return "CREATED";
        case HTTP_STATUS::ACCEPTED: return "Accepted";
        case HTTP_STATUS::NO_CONTENT: return "No Content";
        case HTTP_STATUS::MOVED_PERMANENTLY: return "Moved Permanently";
        case HTTP_STATUS::BAD_REQUEST: return "Bad Request";
        case HTTP_STATUS::FORBIDDEN: return "Forbidden";
        case HTTP_STATUS::NOT_FOUND: return "Not Found";
        case HTTP_STATUS::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_STATUS::REQUEST_IS_BIG:
        case HTTP_STATUS::URI_TOO_LONG:
        case HTTP_STATUS::INTERNAL_SERVER_ERROR: return "Internal Server Error";
          break;
        }
}

void CleanupHttpRequest(HttpResponse& response)
{
    if(response.Content != NULL)
    {
        free(response.Content);
        response.Content = NULL;
    }
}
