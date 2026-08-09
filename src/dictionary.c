#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dictionary.h"
#include "input.h"

/**
 * @brief Helper for case-insensitive string comparison using direct pointer arithmetic.
 * 
 * @param str1 Pointer to first string.
 * @param str2 Pointer to second string.
 * @return int Negative if str1 < str2, positive if str1 > str2, 0 if equal.
 */
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

/**
 * @brief Safely duplicates a null-terminated string with heap allocation.
 * 
 * @param source Pointer to original string to duplicate.
 * @return char* Dynamically allocated string copy, or NULL on allocation failure/NULL input.
 * @note Memory Ownership: The caller assumes full ownership of the returned buffer
 *       and is responsible for deallocating it.
 */
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

/**
 * @brief Allocates and initializes an empty Dictionary hash table structure.
 * 
 * @param bucketCount Number of hash table buckets to allocate.
 * @return Dictionary* Pointer to the newly allocated Dictionary, or NULL on failure.
 * @note Memory Ownership: Allocates the Dictionary struct and bucket pointer array.
 *       All bucket array slots are initialized using direct pointer arithmetic.
 */
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
        *(dictionary->buckets + i) = NULL;
    }

    dictionary->bucketCount = bucketCount;
    dictionary->entryCount = 0;
    return dictionary;
}

/**
 * @brief Computes a case-insensitive DJB2 hash value for a word string.
 * 
 * @param word The word string to hash.
 * @param bucketCount Total bucket count for modulo wrapping.
 * @return size_t Calculated bucket index in range [0, bucketCount - 1], or 0 on error.
 */
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

/**
 * @brief Deallocates all dynamic memory associated with the Dictionary and its entries.
 * 
 * @param dictionary Pointer to the Dictionary instance to destroy.
 * @note Memory Safety: Safely walks each bucket chain using pointer arithmetic,
 *       caching the next pointer before freeing all 4 strings and the node to avoid use-after-free.
 */
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
            DictionaryEntry *current = *(dictionary->buckets + i);
            while (current != NULL)
            {
                /* Memory Safety: Cache next pointer before freeing current node */
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

/**
 * @brief Finds a word entry within the dictionary using case-insensitive lookup.
 * 
 * @param dictionary Const pointer to the Dictionary instance.
 * @param word The word to find.
 * @return DictionaryEntry* Pointer to matching node, or NULL if not found / invalid input.
 * @note Scoped Lookup: Traverses only the target bucket chain via direct pointer arithmetic.
 */
DictionaryEntry *findWord(const Dictionary *dictionary, const char *word)
{
    if (dictionary == NULL || word == NULL || dictionary->buckets == NULL || dictionary->bucketCount == 0)
    {
        return NULL;
    }

    size_t index = hashWord(word, dictionary->bucketCount);
    DictionaryEntry *current = *(dictionary->buckets + index);

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

/**
 * @brief Inserts a new word entry and its metadata into the dictionary.
 * 
 * @param dictionary Pointer to the target Dictionary.
 * @param word Word string (must not be empty or blank).
 * @param partOfSpeech Part of speech classification (noun, verb, etc.).
 * @param definition Text definition of the word.
 * @param exampleSentence Example usage sentence.
 * 
 * @retval  1 Successfully added entry.
 * @retval -1 Memory allocation failure or invalid/empty argument.
 * @retval -2 Duplicate word already exists in the dictionary.
 * 
 * @note Memory Ownership & Rollback: Allocates dynamic copies of all four strings.
 *       If any string allocation fails midway, all allocated strings and the node are
 *       freed immediately to guarantee zero memory leakage.
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

    /* Memory Safety: Rollback and release memory if any string allocation failed */
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

    /* Insert at head of the bucket chain using direct pointer arithmetic */
    size_t index = hashWord(word, dictionary->bucketCount);
    newEntry->next = *(dictionary->buckets + index);
    *(dictionary->buckets + index) = newEntry;

    dictionary->entryCount++;
    return 1;
}

/**
 * @brief Interactively prompts to update a specific field of an existing dictionary entry.
 * 
 * @param dictionary Pointer to the target Dictionary.
 * @param word Word string to locate and update.
 * 
 * @retval  1 Successfully updated field.
 * @retval  0 Word not found or update cancelled by user.
 * @retval -1 Memory allocation failure or invalid input.
 * 
 * @note Memory Safety (Allocate-Before-Free): The new string is allocated and verified
 *       BEFORE the existing field is freed. This ensures existing data is never corrupted
 *       or lost if memory allocation fails.
 */
int updateWord(Dictionary *dictionary, const char *word)
{
    if (dictionary == NULL || word == NULL)
    {
        printf("Error: Invalid dictionary or word parameter.\n");
        return -1;
    }

    DictionaryEntry *entry = findWord(dictionary, word);
    if (entry == NULL)
    {
        printf("Word '%s' not found in dictionary.\n", word);
        return 0;
    }

    printf("\n=== Current Word Entry ===\n");
    printf("Word: %s\n", entry->word != NULL ? entry->word : "");
    printf("Part of Speech: %s\n", entry->partOfSpeech != NULL ? entry->partOfSpeech : "");
    printf("Definition: %s\n", entry->definition != NULL ? entry->definition : "");
    printf("Example Sentence: %s\n", entry->exampleSentence != NULL ? entry->exampleSentence : "");

    printf("\nSelect field to update:\n");
    printf("1. Part of Speech\n");
    printf("2. Definition\n");
    printf("3. Example Sentence\n");
    printf("4. Cancel\n");
    printf("Enter choice (1-4): ");

    int choice = readMenuChoice();
    if (choice < 1 || choice > 3)
    {
        printf("Update cancelled.\n");
        return 0;
    }

    char inputBuffer[1024] = {0};
    printf("Enter new value: ");
    if (!readLine(inputBuffer, sizeof(inputBuffer)) || isBlank(inputBuffer))
    {
        printf("Error: New field value cannot be empty.\n");
        return -1;
    }

    /* Memory Safety: Allocate new string BEFORE freeing old one to safeguard existing data */
    char *newVal = duplicateString(inputBuffer);
    if (newVal == NULL)
    {
        printf("Error: Memory allocation failed during update. Existing data preserved.\n");
        return -1;
    }

    switch (choice)
    {
        case 1:
            free(entry->partOfSpeech);
            entry->partOfSpeech = newVal;
            break;
        case 2:
            free(entry->definition);
            entry->definition = newVal;
            break;
        case 3:
            free(entry->exampleSentence);
            entry->exampleSentence = newVal;
            break;
        default:
            free(newVal);
            return 0;
    }

    printf("Word '%s' updated successfully.\n", entry->word);
    return 1;
}

/**
 * @brief Removes a word entry from the dictionary and deallocates all associated memory.
 * 
 * @param dictionary Pointer to the target Dictionary.
 * @param word The word string to delete.
 * 
 * @retval  1 Successfully deleted.
 * @retval  0 Word not found or user cancelled deletion at prompt.
 * @retval -1 Invalid parameter.
 * 
 * @note Node Unlinking: Unlinks node from chain (head, middle, or tail) via pointer arithmetic,
 *       prompts for confirmation, frees all 4 dynamic strings + node struct, and decrements entryCount.
 */
int deleteWord(Dictionary *dictionary, const char *word)
{
    if (dictionary == NULL || word == NULL || dictionary->buckets == NULL || dictionary->bucketCount == 0)
    {
        printf("Error: Invalid dictionary or word parameter.\n");
        return -1;
    }

    size_t index = hashWord(word, dictionary->bucketCount);
    DictionaryEntry *current = *(dictionary->buckets + index);
    DictionaryEntry *prev = NULL;

    while (current != NULL)
    {
        if (current->word != NULL && caseInsensitiveCompare(current->word, word) == 0)
        {
            break;
        }
        prev = current;
        current = current->next;
    }

    if (current == NULL)
    {
        printf("Word '%s' not found in dictionary.\n", word);
        return 0;
    }

    /* Confirmation prompt */
    char confirmBuffer[16] = {0};
    printf("Are you sure you want to delete '%s'? (y/n): ", current->word);
    if (!readLine(confirmBuffer, sizeof(confirmBuffer)) ||
        (tolower((unsigned char)confirmBuffer[0]) != 'y'))
    {
        printf("Deletion cancelled.\n");
        return 0;
    }

    /* Unlink node from bucket chain */
    if (prev == NULL)
    {
        *(dictionary->buckets + index) = current->next;
    }
    else
    {
        prev->next = current->next;
    }

    /* Free all dynamically allocated strings and the entry struct */
    free(current->word);
    free(current->partOfSpeech);
    free(current->definition);
    free(current->exampleSentence);
    free(current);

    if (dictionary->entryCount > 0)
    {
        dictionary->entryCount--;
    }

    printf("Word '%s' deleted successfully.\n", word);
    return 1;
}
