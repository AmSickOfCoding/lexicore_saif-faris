/*
 * file_manager.c - Loading and saving the dictionary file for LexiCore.
 *
 * File format, one entry per line:
 *     word|part of speech|definition|example sentence
 *
 * A single bad line never stops a load. The line is reported, ignored, and
 * reading carries on with the next one.
 */

#include "file_manager.h"

#include <stdio.h>
#include <string.h>

#include "input.h"

/* Longest line this reader accepts, including the newline and terminator. */
#define LOAD_LINE_BUFFER_SIZE 4096

/* A valid record has exactly four fields separated by three pipes. */
#define FIELD_COUNT 4

/* Room for a short yes or no answer at the overwrite prompt. */
#define ANSWER_BUFFER_SIZE 16

/* Returned by the two public functions in this file. */
#define FILE_OPERATION_SUCCESS 1
#define FILE_OPERATION_FAILURE 0

/*
 * Removes leading and trailing spaces and tabs from a field, in place.
 *
 * Returns a pointer to the first useful character. That pointer points
 * inside the caller's own line buffer, so nothing is allocated here and
 * the caller must not free the result.
 */
static char *trimSpaces(char *text)
{
    char *start = NULL;
    size_t length = 0;

    /* Never walk the string before checking the pointer. */
    if (text == NULL)
    {
        return NULL;
    }

    start = text;
    while (*start == ' ' || *start == '\t')
    {
        start = start + 1;
    }

    length = strlen(start);
    while (length > 0 && (start[length - 1] == ' ' || start[length - 1] == '\t'))
    {
        start[length - 1] = '\0';
        length = length - 1;
    }

    return start;
}

/*
 * Splits a line on '|' by writing a terminator over each separator.
 *
 * Fills the fields array with pointers into the caller's line buffer, so
 * again nothing is allocated and nothing needs freeing. Returns how many
 * fields were found. If there are more than maxFields, it returns
 * maxFields + 1 so the caller can tell the line apart as malformed.
 */
static int splitIntoFields(char *line, char *fields[], int maxFields)
{
    char *current = NULL;
    char *separator = NULL;
    int count = 0;

    if (line == NULL || fields == NULL || maxFields <= 0)
    {
        return 0;
    }

    current = line;
    for (;;)
    {
        /* Text is still left but every slot is used, so there are too many. */
        if (count == maxFields)
        {
            return maxFields + 1;
        }

        separator = strchr(current, '|');
        fields[count] = current;
        count = count + 1;

        if (separator == NULL)
        {
            break;
        }

        /* Cut the line here; the next field starts after the separator. */
        *separator = '\0';
        current = separator + 1;
    }

    return count;
}

/*
 * Reports whether a line was too long to fit in the reading buffer.
 *
 * Returns 1 when characters belonging to this line are still waiting in
 * the file, and throws those characters away so the next read starts at a
 * real line. Returns 0 when the whole line fitted, including the case of
 * a final line that simply has no newline at the end of the file.
 */
static int lineWasTooLong(FILE *file, const char *line)
{
    size_t length = 0;
    int character = 0;

    if (file == NULL || line == NULL)
    {
        return 0;
    }

    length = strlen(line);

    /* A stored newline proves the whole line fitted. */
    if (length > 0 && line[length - 1] == '\n')
    {
        return 0;
    }

    character = fgetc(file);
    if (character == EOF)
    {
        /* The file simply ended without a final newline. */
        return 0;
    }

    /* The rest of this over-long line is unusable, so discard it. */
    while (character != '\n' && character != EOF)
    {
        character = fgetc(file);
    }

    return 1;
}

/*
 * Returns 1 when all four fields hold real text, and 0 when any is empty.
 */
static int allFieldsPresent(char *fields[], int fieldCount)
{
    int index = 0;

    if (fields == NULL)
    {
        return 0;
    }

    for (index = 0; index < fieldCount; index = index + 1)
    {
        if (isBlank(fields[index]) == 1)
        {
            return 0;
        }
    }

    return 1;
}

/*
 * Loads dictionary entries from a text file.
 *
 * Returns FILE_OPERATION_SUCCESS once the file has been read, even when
 * some lines were rejected, and FILE_OPERATION_FAILURE when the file could
 * not be opened at all. The dictionary owns every entry it stores; this
 * function allocates nothing itself and frees nothing. The line buffer
 * lives on the stack and disappears when the function returns.
 */
int loadDictionaryFromFile(Dictionary *dictionary, const char *filename)
{
    char line[LOAD_LINE_BUFFER_SIZE] = {0};
    char *fields[FIELD_COUNT] = {NULL};
    FILE *file = NULL;
    size_t lineNumber = 0;
    size_t validCount = 0;
    size_t invalidCount = 0;
    size_t duplicateCount = 0;
    int fieldCount = 0;
    int index = 0;

    /* Check both pointers before using either of them. */
    if (dictionary == NULL || filename == NULL)
    {
        printf("Error: No dictionary or no filename was given.\n");
        return FILE_OPERATION_FAILURE;
    }

    file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error: Could not open dictionary file '%s'.\n", filename);
        return FILE_OPERATION_FAILURE;
    }

    while (fgets(line, (int)sizeof(line), file) != NULL)
    {
        lineNumber = lineNumber + 1;

        /* A line longer than the buffer cannot be trusted, so reject it. */
        if (lineWasTooLong(file, line) == 1)
        {
            printf("Warning: Invalid entry ignored at line %zu.\n", lineNumber);
            invalidCount = invalidCount + 1;
            continue;
        }

        trimNewline(line);

        /* Blank lines are skipped without any message. */
        if (isBlank(line) == 1)
        {
            continue;
        }

        fieldCount = splitIntoFields(line, fields, FIELD_COUNT);
        if (fieldCount != FIELD_COUNT)
        {
            printf("Warning: Invalid entry ignored at line %zu.\n", lineNumber);
            invalidCount = invalidCount + 1;
            continue;
        }

        /* Each trimmed field still points inside the line buffer. */
        for (index = 0; index < FIELD_COUNT; index = index + 1)
        {
            fields[index] = trimSpaces(fields[index]);
        }

        if (allFieldsPresent(fields, FIELD_COUNT) == 0)
        {
            printf("Warning: Invalid entry ignored at line %zu.\n", lineNumber);
            invalidCount = invalidCount + 1;
            continue;
        }

        /*
         * Look the word up first. A word already in the table is an ignored
         * record, not an error, so it is counted on its own.
         */
        if (findWord(dictionary, fields[0]) != NULL)
        {
            duplicateCount = duplicateCount + 1;
            continue;
        }

        /* Field order in the file is word, part of speech, definition, example. */
        addWord(dictionary, fields[0], fields[1], fields[2], fields[3]);

        /*
         * Confirm the word really is in the table now. Checking the result
         * this way keeps the loader working whatever number addWord returns
         * to mean success.
         */
        if (findWord(dictionary, fields[0]) == NULL)
        {
            printf("Warning: Entry at line %zu could not be stored.\n", lineNumber);
            invalidCount = invalidCount + 1;
            continue;
        }

        validCount = validCount + 1;
    }

    /* The file is closed on every path out of this function. */
    fclose(file);

    printf("Dictionary loaded successfully.\n");
    printf("Valid entries loaded: %zu\n", validCount);
    printf("Invalid lines ignored: %zu\n", invalidCount);
    printf("Duplicate words ignored: %zu\n", duplicateCount);

    return FILE_OPERATION_SUCCESS;
}

/*
 * Returns the string itself, or an empty string when it is NULL.
 *
 * Passing NULL to printf with %s is undefined behaviour, so every field is
 * sent through this guard before it is written.
 */
static const char *textOrEmpty(const char *text)
{
    if (text == NULL)
    {
        return "";
    }

    return text;
}

/*
 * Writes every entry in the table to an already open file.
 *
 * Returns 1 when all entries were written and 0 on the first write error.
 * The number written is stored through writtenCount, which the caller owns.
 */
static int writeAllEntries(const Dictionary *dictionary, FILE *file,
                           size_t *writtenCount)
{
    DictionaryEntry *current = NULL;
    size_t bucketIndex = 0;
    int printResult = 0;

    if (dictionary == NULL || file == NULL || writtenCount == NULL)
    {
        return 0;
    }

    *writtenCount = 0;

    /* Walk every bucket, then every entry chained inside that bucket. */
    for (bucketIndex = 0; bucketIndex < dictionary->bucketCount; bucketIndex = bucketIndex + 1)
    {
        current = dictionary->buckets[bucketIndex];
        while (current != NULL)
        {
            printResult = fprintf(file, "%s|%s|%s|%s\n",
                                  textOrEmpty(current->word),
                                  textOrEmpty(current->partOfSpeech),
                                  textOrEmpty(current->definition),
                                  textOrEmpty(current->exampleSentence));

            /* A negative result from fprintf means the write failed. */
            if (printResult < 0)
            {
                return 0;
            }

            *writtenCount = *writtenCount + 1;
            current = current->next;
        }
    }

    return 1;
}

/*
 * Asks the user before an existing file is overwritten.
 *
 * Returns 1 when saving may go ahead, either because the file is new or
 * because the user agreed, and 0 when the file must be left alone. If the
 * answer cannot be read the safe choice is made and nothing is overwritten.
 */
static int confirmOverwrite(const char *filename)
{
    char answer[ANSWER_BUFFER_SIZE] = {0};
    FILE *existingFile = NULL;

    if (filename == NULL)
    {
        return 0;
    }

    /* Opening for reading is how this checks whether the file is already there. */
    existingFile = fopen(filename, "r");
    if (existingFile == NULL)
    {
        /* Nothing exists yet, so there is nothing to overwrite. */
        return 1;
    }

    fclose(existingFile);

    printf("Warning: '%s' already exists and will be overwritten.\n", filename);
    printf("Continue? (y/n): ");
    fflush(stdout);

    if (readLine(answer, sizeof(answer)) == 0)
    {
        return 0;
    }

    if (answer[0] == 'y' || answer[0] == 'Y')
    {
        return 1;
    }

    return 0;
}

/*
 * Saves the whole dictionary to a file in the pipe-separated format.
 *
 * Returns FILE_OPERATION_SUCCESS when the file was written and closed
 * cleanly, and FILE_OPERATION_FAILURE when it was not written. Nothing is
 * allocated here, so nothing is freed; the dictionary keeps owning all of
 * its entries and is only read from.
 */
int saveDictionaryToFile(const Dictionary *dictionary, const char *filename)
{
    FILE *file = NULL;
    size_t writtenCount = 0;
    int writeSucceeded = 0;

    /* Check every pointer before it is followed. */
    if (dictionary == NULL || filename == NULL)
    {
        printf("Error: No dictionary or no filename was given.\n");
        return FILE_OPERATION_FAILURE;
    }

    if (dictionary->buckets == NULL)
    {
        printf("Error: The dictionary has no bucket array to save.\n");
        return FILE_OPERATION_FAILURE;
    }

    /* Ask before destroying a file that is already on disk. */
    if (confirmOverwrite(filename) == 0)
    {
        printf("Save cancelled. '%s' was not changed.\n", filename);
        return FILE_OPERATION_FAILURE;
    }

    file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Error: Could not open '%s' for writing.\n", filename);
        return FILE_OPERATION_FAILURE;
    }

    writeSucceeded = writeAllEntries(dictionary, file, &writtenCount);
    if (writeSucceeded == 0)
    {
        printf("Error: Failed while writing to '%s'.\n", filename);
        fclose(file);
        return FILE_OPERATION_FAILURE;
    }

    /*
     * Closing can still fail because data held in the buffer is written out
     * at that moment, so the result of fclose is checked as well.
     */
    if (fclose(file) != 0)
    {
        printf("Error: Failed while closing '%s'. The file may be incomplete.\n", filename);
        return FILE_OPERATION_FAILURE;
    }

    printf("Dictionary saved successfully.\n");
    printf("Entries written: %zu\n", writtenCount);

    return FILE_OPERATION_SUCCESS;
}
