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
DictionaryEntry *findWord(const Dictionary *dictionary, const char *word);
int addWord(Dictionary *dictionary, const char *word, const char *partOfSpeech,
            const char *definition, const char *exampleSentence);
int updateWord(Dictionary *dictionary, const char *word);
int deleteWord(Dictionary *dictionary, const char *word);
size_t findWordsByPrefix(const Dictionary *dictionary, const char *prefix);
void displayDictionaryAlphabetically(const Dictionary *dictionary);
void destroyDictionary(Dictionary *dictionary);

#endif