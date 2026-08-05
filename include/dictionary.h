#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stddef.h>

typedef struct DictionaryEntry
{
    char *word;
    char *definition;
    char *partOfSpeech;
    char *exampleSentence;
    struct DictionaryEntry *next;
} DictionaryEntry;

typedef struct
{
    DictionaryEntry **buckets;
    size_t bucketCount;
    size_t entryCount;
} Dictionary;

char *duplicateString(const char *source);
Dictionary *createDictionary(size_t bucketCount);
size_t hashWord(const char *word, size_t bucketCount);
void destroyDictionary(Dictionary *dictionary);

#endif