#pragma once
#include <cstdint>
#include <stdint.h>
#include <unistd.h>

#define MAX_HTTP_REQUEST_SIZE 8192

enum class HTTP_STATUS : uint16_t
{
    // Successful Responses
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,

    // Redirection Responses
    MOVED_PERMANENTLY = 301,
    //
    // Client Error Responses
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    REQUEST_IS_BIG = 413,

    // Server Error Responses
    INTERNAL_SERVER_ERROR = 500,
};
enum class HTTP_READ_STATUS : uint8_t
{
    SUCCESS,
    ERROR,
    CLIENT_CLOSED_CONNECTION,
    BUFFER_FULL,
};

enum class HTTP_METHOD : uint8_t
{
    GET,
    POST,
    PUT,
    DELETE,
    NONE,
};
struct RequestLine
{
    char URL[2048];
    char HttpVersion[16];
    HTTP_METHOD Method;
};
struct RequestHeader
{
    char Name[64];
    char Value[256];
};
struct HttpRequest
{
    RequestLine ReqLine;
    RequestHeader Headers[50];
};
struct HttpResponse 
{
    HTTP_STATUS Status;
    char* HtmlFile;
    long ContentSize;
    
};

HTTP_READ_STATUS ReadHttpRequest(int sockfd, char* outRequestBuffer, int &outTotalBytesRead);
bool ParseHttpRequest(HttpRequest& request, char* requestBuffer);
HTTP_METHOD  GetHttpMethodFromStr(const char* methodStr);

HttpResponse HandleHttpRequest(HttpRequest& request);
