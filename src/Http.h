#pragma once
#include <stdint.h>

#define MAX_HTTP_REQUEST_SIZE 8192

enum class HTML_STATUS : uint16_t
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

enum class HTTP_METHOD
{
    GET,
    POST,
    PUT,
    DELETE,
    NONE

};
struct RequestLine
{
    char* URL;
    HTTP_METHOD Method;
    int HttpVersionMajor;
    int HttpVersionMinor;
};
struct HttpRequest
{
    RequestLine RequestLine;
    
};


HTTP_METHOD  GetHttpMethodFromStr(const char* methodStr);
