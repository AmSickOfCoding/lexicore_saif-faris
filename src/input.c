#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "input.h"

void trimNewline(char *str)
{
    if (str == NULL) return;
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r'))
    {
        str[len - 1] = '\0';
        len--;
    }
}

int isBlank(const char *str)
{
    if (str == NULL) return 1;
    while (*str)
    {
        if (!isspace((unsigned char)*str))
        {
            return 0;
        }
        str++;
    }
    return 1;
}

int readLine(char *buffer, size_t bufferSize)
{
    if (buffer == NULL || bufferSize == 0)
    {
        return 0;
    }

    if (fgets(buffer, (int)bufferSize, stdin) == NULL)
    {
        return 0;
    }

    trimNewline(buffer);
    return 1;
}

int readMenuChoice(void)
{
    char buffer[64];
    if (!readLine(buffer, sizeof(buffer)))
    {
        return -1;
    }
    if (isBlank(buffer))
    {
        return -1;
    }
    return atoi(buffer);
}
