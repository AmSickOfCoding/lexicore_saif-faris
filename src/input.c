/*
 * input.c - Safe console input helpers for LexiCore.
 *
 * Every function in this file is written so that a NULL pointer or an
 * empty string can never cause a crash. gets() is never used.
 */

#include "input.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A menu choice is a short number, so a small local buffer is plenty. */
#define MENU_INPUT_BUFFER_SIZE 64

/* Returned when the user typed something that is not a menu number. */
#define MENU_CHOICE_INVALID (-1)

/*
 * Reads and throws away the characters left over from a line that was too
 * long to fit in the caller's buffer, up to and including the newline.
 *
 * Without this the leftover characters would be picked up by the next
 * read and would look to the program like a second, bogus line.
 */
static void discardRestOfLine(void)
{
    int character = 0;

    character = getchar();
    while (character != '\n' && character != EOF)
    {
        character = getchar();
    }
}

/*
 * Reads one line of text from stdin into the caller's buffer.
 *
 * Returns 1 on success and 0 on failure (end of file, a read error, or a
 * buffer the caller did not supply). The buffer belongs to the caller and
 * is never allocated or freed here. At most bufferSize - 1 characters are
 * stored, so the buffer can never overflow. The trailing newline is
 * removed before returning.
 */
int readLine(char *buffer, size_t bufferSize)
{
    char *readResult = NULL;
    size_t length = 0;

    /* Never write through the pointer before checking it is usable. */
    if (buffer == NULL || bufferSize == 0)
    {
        return 0;
    }

    /* Leave the buffer as a valid empty string on every failure path. */
    buffer[0] = '\0';

    /* fgets takes the size as an int, so refuse an impossibly large size. */
    if (bufferSize > (size_t)INT_MAX)
    {
        return 0;
    }

    readResult = fgets(buffer, (int)bufferSize, stdin);
    if (readResult == NULL)
    {
        buffer[0] = '\0';
        return 0;
    }

    length = strlen(buffer);

    /*
     * A stored newline means the whole line fitted. If it is missing, the
     * line was longer than the buffer and the rest must be discarded.
     */
    if (length == 0 || buffer[length - 1] != '\n')
    {
        discardRestOfLine();
    }

    trimNewline(buffer);
    return 1;
}

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

/*
 * Reads one whole line and converts it to a menu number.
 *
 * The line is always consumed, whatever it contains, so letters or an
 * empty line can never leave anything behind in stdin and can never make
 * the caller's menu loop spin forever. Returns the number the user typed,
 * or MENU_CHOICE_INVALID (-1) when the line was not a plain number. The
 * caller prints the prompt and decides whether to ask again.
 */
int readMenuChoice(void)
{
    char buffer[MENU_INPUT_BUFFER_SIZE] = {0};
    char *firstUnusedCharacter = NULL;
    long value = 0;

    /* A failed read means end of input, not a choice. */
    if (readLine(buffer, sizeof(buffer)) == 0)
    {
        return MENU_CHOICE_INVALID;
    }

    if (isBlank(buffer) == 1)
    {
        return MENU_CHOICE_INVALID;
    }

    /* strtol reports a too-large number by setting errno to ERANGE. */
    errno = 0;
    value = strtol(buffer, &firstUnusedCharacter, 10);

    if (errno == ERANGE)
    {
        return MENU_CHOICE_INVALID;
    }

    /* Nothing was converted, so the line started with a letter or sign only. */
    if (firstUnusedCharacter == buffer)
    {
        return MENU_CHOICE_INVALID;
    }

    /* Trailing spaces are fine; anything else means input like "3x". */
    while (*firstUnusedCharacter == ' ' || *firstUnusedCharacter == '\t')
    {
        firstUnusedCharacter = firstUnusedCharacter + 1;
    }

    if (*firstUnusedCharacter != '\0')
    {
        return MENU_CHOICE_INVALID;
    }

    /* Menu numbers are small and positive, so reject anything outside that. */
    if (value < 0 || value > (long)INT_MAX)
    {
        return MENU_CHOICE_INVALID;
    }

    return (int)value;
}
