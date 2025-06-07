#include "Http.h"
#include <string.h>


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

