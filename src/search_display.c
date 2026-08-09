/*
 * search_display.c - Prefix search and the alphabetical listing for LexiCore.
 *
 * Both functions here only ever read the dictionary. No word, definition,
 * part of speech, or example sentence is ever copied or freed in this file:
 * the dictionary keeps owning all of that memory, and everything below just
 * borrows a pointer to it long enough to print it.
 */

#include "dictionary.h"

#include <ctype.h>
#include <stdio.h>

/* Printed in place of a field that was never filled in. */
#define MISSING_FIELD_TEXT "(none)"

/*
 * Returns the text of a field, or a placeholder when the field is NULL.
 *
 * printf("%s", NULL) is undefined behaviour, so every field goes through
 * this before it is printed. The returned pointer is either the caller's
 * own text or a string literal; either way nothing is allocated here.
 */
static const char *textOrPlaceholder(const char *text)
{
    /* Never hand a NULL pointer to printf. */
    if (text == NULL)
    {
        return MISSING_FIELD_TEXT;
    }

    return text;
}

/*
 * Returns 1 when word begins with prefix, ignoring upper and lower case,
 * and 0 otherwise.
 *
 * Both strings belong to the caller and are only read. A NULL pointer on
 * either side counts as "no match" rather than as a crash.
 */
static int startsWithIgnoreCase(const char *word, const char *prefix)
{
    size_t index = 0;
    int wordCharacter = 0;
    int prefixCharacter = 0;

    /* Neither string can be read before both pointers are checked. */
    if (word == NULL || prefix == NULL)
    {
        return 0;
    }

    while (prefix[index] != '\0')
    {
        /* The word ended before the prefix did, so it cannot match. */
        if (word[index] == '\0')
        {
            return 0;
        }

        /* tolower is only safe on a value that fits in an unsigned char. */
        wordCharacter = tolower((unsigned char)word[index]);
        prefixCharacter = tolower((unsigned char)prefix[index]);

        if (wordCharacter != prefixCharacter)
        {
            return 0;
        }

        index = index + 1;
    }

    /* Every character of the prefix matched, so the word starts with it. */
    return 1;
}

/*
 * Prints every word that starts with the given prefix and returns how many
 * were found.
 *
 * EVERY bucket has to be looked at. The hash function turns a whole word
 * into one bucket number, so words that share a prefix do not share a
 * bucket: "program" and "process" can easily land at opposite ends of the
 * table. There is no way to work out from the prefix alone which buckets
 * could hold a match, so the hash cannot shortcut this search and the whole
 * table is walked from bucket 0 to the last one.
 *
 * Nothing is allocated in this function, so there is nothing to free. A
 * NULL dictionary and a NULL or empty prefix all return 0 without crashing.
 */
size_t findWordsByPrefix(const Dictionary *dictionary, const char *prefix)
{
    DictionaryEntry *current = NULL;
    size_t bucketIndex = 0;
    size_t matchCount = 0;

    /* Never follow the pointer before checking that it is real. */
    if (dictionary == NULL)
    {
        printf("Error: There is no dictionary to search.\n");
        return 0;
    }

    /* The walk below reads the bucket array, so that pointer is checked too. */
    if (dictionary->buckets == NULL)
    {
        printf("Error: The dictionary has no words to search.\n");
        return 0;
    }

    /* An empty prefix would match every word, which is not a search. */
    if (prefix == NULL || prefix[0] == '\0')
    {
        printf("Error: Please enter a prefix to search for.\n");
        return 0;
    }

    printf("\n");

    /* Walk every bucket, because a prefix does not hash to a single one. */
    for (bucketIndex = 0; bucketIndex < dictionary->bucketCount; bucketIndex = bucketIndex + 1)
    {
        /* current only ever borrows an entry the dictionary owns. */
        current = dictionary->buckets[bucketIndex];
        while (current != NULL)
        {
            if (startsWithIgnoreCase(current->word, prefix) == 1)
            {
                matchCount = matchCount + 1;
                printf("%zu. %s\n", matchCount, textOrPlaceholder(current->word));
            }

            /* Step along the chain; no entry is changed or freed here. */
            current = current->next;
        }
    }

    if (matchCount == 0)
    {
        printf("No matching words found.\n");
        return 0;
    }

    if (matchCount == 1)
    {
        printf("\n1 matching word found.\n");
        return matchCount;
    }

    printf("\n%zu matching words found.\n", matchCount);
    return matchCount;
}
