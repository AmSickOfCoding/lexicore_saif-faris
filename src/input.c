/*
 * input.c - Safe console input helpers for LexiCore.
 *
 * Every function in this file is written so that a NULL pointer or an
 * empty string can never cause a crash. gets() is never used.
 */

#include "input.h"

#include <stdio.h>
#include <string.h>

/*
 * Removes a trailing newline from a string, in place.
 *
 * Does nothing when str is NULL, when the string is empty, or when there
 * is no newline at the end. A trailing carriage return is removed as well
 * so that files written on Windows (which end lines with "\r\n") do not
 * leave a stray '\r' at the end of a field.
 */
void trimNewline(char *str)
{
    size_t length = 0;

    /* Never dereference the pointer before checking it. */
    if (str == NULL)
    {
        return;
    }

    length = strlen(str);

    /* An empty string has no last character to look at. */
    if (length == 0)
    {
        return;
    }

    if (str[length - 1] == '\n')
    {
        str[length - 1] = '\0';
        length = length - 1;
    }

    /* A Windows line ending leaves a '\r' behind once the '\n' is gone. */
    if (length > 0 && str[length - 1] == '\r')
    {
        str[length - 1] = '\0';
    }
}

/*
 * Returns 1 when the string carries no useful text, and 0 otherwise.
 *
 * A NULL pointer, an empty string, and a string made only of spaces and
 * tabs are all treated as blank. This is what rejects empty input fields.
 */
int isBlank(const char *str)
{
    size_t index = 0;

    /* A missing string counts as blank rather than as an error. */
    if (str == NULL)
    {
        return 1;
    }

    while (str[index] != '\0')
    {
        if (str[index] != ' ' && str[index] != '\t')
        {
            return 0;
        }
        index = index + 1;
    }

    return 1;
}
