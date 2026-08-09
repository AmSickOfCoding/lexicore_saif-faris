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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

/*
 * Compares two strings ignoring upper and lower case, the way strcmp does.
 *
 * Returns a negative number when left sorts first, 0 when the two are the
 * same word, and a positive number when right sorts first. A NULL string
 * is treated as sorting before every real one so that a missing word field
 * can never be dereferenced. Both strings are only read.
 */
static int compareIgnoreCase(const char *left, const char *right)
{
    size_t index = 0;
    int leftCharacter = 0;
    int rightCharacter = 0;

    /* Sort any missing string to the front instead of reading it. */
    if (left == NULL && right == NULL)
    {
        return 0;
    }

    if (left == NULL)
    {
        return -1;
    }

    if (right == NULL)
    {
        return 1;
    }

    while (left[index] != '\0' && right[index] != '\0')
    {
        /* tolower is only safe on a value that fits in an unsigned char. */
        leftCharacter = tolower((unsigned char)left[index]);
        rightCharacter = tolower((unsigned char)right[index]);

        if (leftCharacter != rightCharacter)
        {
            return leftCharacter - rightCharacter;
        }

        index = index + 1;
    }

    /*
     * One of the two ran out first. Comparing the characters that stopped
     * the loop puts the shorter word first, and gives 0 when both ended
     * together, because the end marker '\0' counts as zero.
     */
    leftCharacter = tolower((unsigned char)left[index]);
    rightCharacter = tolower((unsigned char)right[index]);

    return leftCharacter - rightCharacter;
}

/*
 * The comparison qsort uses to order the temporary array.
 *
 * qsort hands over the address of each slot rather than the entry itself,
 * so the entry pointer has to be read out of the slot before the words can
 * be compared. Only the words are looked at, and no entry is moved,
 * changed, or freed here; qsort reorders the pointers in the array only.
 */
static int compareEntriesByWord(const void *leftSlot, const void *rightSlot)
{
    DictionaryEntry *const *leftAddress = (DictionaryEntry *const *)leftSlot;
    DictionaryEntry *const *rightAddress = (DictionaryEntry *const *)rightSlot;
    const DictionaryEntry *leftEntry = NULL;
    const DictionaryEntry *rightEntry = NULL;

    /* Read the borrowed entry pointer out of each slot. */
    leftEntry = *leftAddress;
    rightEntry = *rightAddress;

    /* An empty slot should never reach here, but is handled all the same. */
    if (leftEntry == NULL || rightEntry == NULL)
    {
        return compareIgnoreCase(NULL, NULL);
    }

    return compareIgnoreCase(leftEntry->word, rightEntry->word);
}

/*
 * Prints one numbered line of the alphabetical listing.
 *
 * The entry is only read. Every field goes through textOrPlaceholder so a
 * field that was never filled in cannot reach printf as a NULL pointer.
 */
static void printEntryLine(size_t position, const DictionaryEntry *entry)
{
    /* Never follow the pointer before checking that it is real. */
    if (entry == NULL)
    {
        return;
    }

    printf("%4zu. %-18s %-12s %s\n",
           position,
           textOrPlaceholder(entry->word),
           textOrPlaceholder(entry->partOfSpeech),
           textOrPlaceholder(entry->definition));
}

/*
 * Stores a pointer to every entry in the table in the caller's array.
 *
 * Only the pointers are copied. Copying the entries themselves would mean
 * duplicating four strings for every word, which would double the memory
 * the program uses and would leave two owners for text that only the
 * dictionary is allowed to free. Borrowing the pointers keeps ownership in
 * one place: the array is thrown away afterwards and the entries live on.
 *
 * Returns how many pointers were stored. The count never goes past
 * capacity, so a stale entryCount in the struct cannot cause a write past
 * the end of the array.
 */
static size_t collectEntryPointers(const Dictionary *dictionary,
                                   DictionaryEntry **entries,
                                   size_t capacity)
{
    DictionaryEntry *current = NULL;
    size_t bucketIndex = 0;
    size_t stored = 0;

    /* Both pointers are used below, so neither is trusted unchecked. */
    if (dictionary == NULL || dictionary->buckets == NULL || entries == NULL)
    {
        return 0;
    }

    for (bucketIndex = 0; bucketIndex < dictionary->bucketCount; bucketIndex = bucketIndex + 1)
    {
        /* current only ever borrows an entry the dictionary owns. */
        current = dictionary->buckets[bucketIndex];
        while (current != NULL && stored < capacity)
        {
            /* Store the address of the entry, not a copy of the entry. */
            entries[stored] = current;
            stored = stored + 1;

            /* Step along the chain; no entry is changed or freed here. */
            current = current->next;
        }
    }

    return stored;
}

/*
 * Prints every word in the dictionary in alphabetical order.
 *
 * The entries live in hash order, which is not alphabetical, so they are
 * sorted through a temporary array of borrowed pointers. That array is the
 * only thing allocated here and it is freed on every way out of the
 * function, including the early returns. The entries themselves are never
 * copied and never freed; the dictionary still owns them afterwards.
 *
 * A NULL dictionary, an empty dictionary, and a failed allocation each
 * print a plain message instead of crashing.
 */
void displayDictionaryAlphabetically(const Dictionary *dictionary)
{
    DictionaryEntry **entries = NULL;
    size_t entryCount = 0;
    size_t stored = 0;
    size_t index = 0;

    /* Never follow the pointer before checking that it is real. */
    if (dictionary == NULL)
    {
        printf("Error: There is no dictionary to display.\n");
        return;
    }

    /* The walk below reads the bucket array, so that pointer is checked too. */
    if (dictionary->buckets == NULL)
    {
        printf("The dictionary is empty. There is nothing to display.\n");
        return;
    }

    entryCount = dictionary->entryCount;

    /* An empty dictionary needs no array, so nothing is allocated at all. */
    if (entryCount == 0)
    {
        printf("The dictionary is empty. There is nothing to display.\n");
        return;
    }

    /*
     * Refuse a count so large that working out the size in bytes would wrap
     * around, because that would allocate far less memory than it looks.
     */
    if (entryCount > SIZE_MAX / sizeof(DictionaryEntry *))
    {
        printf("Error: The dictionary is too large to sort.\n");
        return;
    }

    /* One slot per entry, each holding a borrowed pointer, not an entry. */
    entries = malloc(entryCount * sizeof(DictionaryEntry *));
    if (entries == NULL)
    {
        printf("Error: Not enough memory to sort the dictionary.\n");
        return;
    }

    stored = collectEntryPointers(dictionary, entries, entryCount);

    /*
     * The struct said there were entries but the chains held none, so the
     * array is empty. It still has to be freed before returning.
     */
    if (stored == 0)
    {
        printf("The dictionary is empty. There is nothing to display.\n");
        free(entries);
        entries = NULL;
        return;
    }

    /* Sorting moves the pointers inside the array; the entries do not move. */
    qsort(entries, stored, sizeof(DictionaryEntry *), compareEntriesByWord);

    printf("\n========== All Words (A-Z) ==========\n");

    for (index = 0; index < stored; index = index + 1)
    {
        printEntryLine(index + 1, entries[index]);
    }

    printf("=====================================\n");
    printf("%zu words listed.\n", stored);

    /* The borrowed pointers are done with, so the array itself is freed. */
    free(entries);
    entries = NULL;
}
