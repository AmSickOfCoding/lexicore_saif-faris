#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dictionary.h"
#include "input.h"

/* Case-insensitive string comparison helper using pointer traversal */
static int caseInsensitiveCompare(const char *str1, const char *str2)
{
    if (str1 == NULL || str2 == NULL)
    {
        return (str1 == str2) ? 0 : (str1 == NULL ? -1 : 1);
    }

    const char *p1 = str1;
    const char *p2 = str2;

    while (*p1 != '\0' && *p2 != '\0')
    {
        int c1 = tolower((unsigned char)*p1);
        int c2 = tolower((unsigned char)*p2);

        if (c1 != c2)
        {
            return c1 - c2;
        }
        p1++;
        p2++;
    }

    return (int)(tolower((unsigned char)*p1) - tolower((unsigned char)*p2));
}

/* Safely duplicates a string with memory allocation failure check */
char *duplicateString(const char *source)
{
    if (source == NULL)
    {
        return NULL;
    }

    size_t length = strlen(source);
    char *copy = malloc(length + 1);

    if (copy == NULL)
    {
        return NULL;
    }

    strcpy(copy, source);
    return copy;
}

/* Allocates and initializes a new dictionary structure and buckets array */
Dictionary *createDictionary(size_t bucketCount)
{
    if (bucketCount == 0)
    {
        return NULL;
    }

    Dictionary *dictionary = malloc(sizeof(Dictionary));
    if (dictionary == NULL)
    {
        return NULL;
    }

    dictionary->buckets = malloc(bucketCount * sizeof(DictionaryEntry *));
    if (dictionary->buckets == NULL)
    {
        free(dictionary);
        return NULL;
    }

    for (size_t i = 0; i < bucketCount; i++)
    {
        dictionary->buckets[i] = NULL;
    }

    dictionary->bucketCount = bucketCount;
    dictionary->entryCount = 0;
    return dictionary;
}

/* Calculates a case-insensitive hash for a word using DJB2 algorithm */
size_t hashWord(const char *word, size_t bucketCount)
{
    if (word == NULL || bucketCount == 0)
    {
        return 0;
    }

    unsigned long hash = 5381;
    int c;

    while ((c = (unsigned char)*word++) != '\0')
    {
        hash = hash * 33 + (unsigned long)tolower(c);
    }

    return (size_t)(hash % bucketCount);
}

/* Frees all dynamic memory associated with the dictionary */
void destroyDictionary(Dictionary *dictionary)
{
    if (dictionary == NULL)
    {
        return;
    }

    if (dictionary->buckets != NULL)
    {
        for (size_t i = 0; i < dictionary->bucketCount; i++)
        {
            DictionaryEntry *current = dictionary->buckets[i];
            while (current != NULL)
            {
                DictionaryEntry *next = current->next;
                free(current->word);
                free(current->partOfSpeech);
                free(current->definition);
                free(current->exampleSentence);
                free(current);
                current = next;
            }
        }
        free(dictionary->buckets);
    }

    free(dictionary);
}

/* Finds a word in the dictionary (case-insensitive) by walking the target bucket */
DictionaryEntry *findWord(const Dictionary *dictionary, const char *word)
{
    if (dictionary == NULL || word == NULL || dictionary->buckets == NULL || dictionary->bucketCount == 0)
    {
        return NULL;
    }

    size_t index = hashWord(word, dictionary->bucketCount);
    DictionaryEntry *current = dictionary->buckets[index];

    while (current != NULL)
    {
        if (current->word != NULL && caseInsensitiveCompare(current->word, word) == 0)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/*
 * Adds a new word entry into the dictionary.
 * Rejects empty fields and duplicate words.
 * Returns:
 *   1  : Success
 *  -1  : Memory allocation failure or invalid argument
 *  -2  : Duplicate word
 */
int addWord(Dictionary *dictionary, const char *word, const char *partOfSpeech,
            const char *definition, const char *exampleSentence)
{
    if (dictionary == NULL || word == NULL || partOfSpeech == NULL ||
        definition == NULL || exampleSentence == NULL)
    {
        return -1;
    }

    if (isBlank(word) || isBlank(partOfSpeech) || isBlank(definition) || isBlank(exampleSentence))
    {
        return -1;
    }

    /* Reject duplicate words (case-insensitive check) */
    if (findWord(dictionary, word) != NULL)
    {
        return -2;
    }

    DictionaryEntry *newEntry = malloc(sizeof(DictionaryEntry));
    if (newEntry == NULL)
    {
        return -1;
    }

    newEntry->word = duplicateString(word);
    newEntry->partOfSpeech = duplicateString(partOfSpeech);
    newEntry->definition = duplicateString(definition);
    newEntry->exampleSentence = duplicateString(exampleSentence);

    /* Rollback memory if any string allocation failed */
    if (newEntry->word == NULL || newEntry->partOfSpeech == NULL ||
        newEntry->definition == NULL || newEntry->exampleSentence == NULL)
    {
        free(newEntry->word);
        free(newEntry->partOfSpeech);
        free(newEntry->definition);
        free(newEntry->exampleSentence);
        free(newEntry);
        return -1;
    }

    size_t index = hashWord(word, dictionary->bucketCount);
    newEntry->next = dictionary->buckets[index];
    dictionary->buckets[index] = newEntry;

    dictionary->entryCount++;
    return 1;
}
