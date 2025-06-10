#pragma once
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define BACKLOG 10

inline void ToLower(char* str)
{
    for(size_t i{}; i < strlen(str); i++)
    {
        str[i] = (char)tolower(str[i]);
    }

}
#define TOLOWER(str) ToLower(str)
