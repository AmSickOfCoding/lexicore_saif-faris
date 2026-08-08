/*
 * statistics.c - The dictionary statistics report for LexiCore.
 *
 * Every number printed here is worked out by walking the hash table at the
 * moment the report is asked for. Nothing is stored between calls and no
 * running total is kept inside the Dictionary struct, so the figures can
 * never drift out of step with what the table actually holds.
 */

#include "statistics.h"

#include <ctype.h>
#include <stdio.h>

/* The five groups every word is counted into. */
#define POS_NOUN 0
#define POS_VERB 1
#define POS_ADJECTIVE 2
#define POS_ADVERB 3
#define POS_OTHER 4
#define POS_CATEGORY_COUNT 5

/*
 * Compares two strings while ignoring upper and lower case.
 *
 * Returns 1 when they hold the same letters and 0 otherwise. Both strings
 * belong to the caller; nothing is allocated, changed, or freed here.
 */
static int equalsIgnoreCase(const char *left, const char *right)
{
    size_t index = 0;
    int leftCharacter = 0;
    int rightCharacter = 0;

    /* Two strings can only be compared when both pointers are real. */
    if (left == NULL || right == NULL)
    {
        return 0;
    }

    while (left[index] != '\0' && right[index] != '\0')
    {
        /* tolower is only safe on a value that fits in an unsigned char. */
        leftCharacter = tolower((unsigned char)left[index]);
        rightCharacter = tolower((unsigned char)right[index]);

        if (leftCharacter != rightCharacter)
        {
            return 0;
        }

        index = index + 1;
    }

    /* Equal so far, so they only match if both ran out at the same place. */
    if (left[index] == '\0' && right[index] == '\0')
    {
        return 1;
    }

    return 0;
}

/*
 * Decides which of the five groups a part of speech label belongs to.
 *
 * Returns one of the POS_ constants. A missing label, or a label that is
 * not one of the four known ones, is counted as POS_OTHER. The string
 * belongs to the entry it came from and is only read here.
 */
static int partOfSpeechCategory(const char *partOfSpeech)
{
    /* A field that was never filled in still has to be counted somewhere. */
    if (partOfSpeech == NULL)
    {
        return POS_OTHER;
    }

    if (equalsIgnoreCase(partOfSpeech, "noun") == 1)
    {
        return POS_NOUN;
    }

    if (equalsIgnoreCase(partOfSpeech, "verb") == 1)
    {
        return POS_VERB;
    }

    if (equalsIgnoreCase(partOfSpeech, "adjective") == 1)
    {
        return POS_ADJECTIVE;
    }

    if (equalsIgnoreCase(partOfSpeech, "adverb") == 1)
    {
        return POS_ADVERB;
    }

    return POS_OTHER;
}

/*
 * Prints one whole-number line of the report with the value right aligned.
 */
static void printCountLine(const char *label, size_t value)
{
    printf("%-26s%9zu\n", label, value);
}

/*
 * Prints one decimal line of the report with the value right aligned.
 */
static void printRatioLine(const char *label, double value)
{
    printf("%-26s%9.2f\n", label, value);
}

/*
 * Prints one indented line of the part of speech breakdown.
 */
static void printPartOfSpeechLine(const char *label, size_t value)
{
    printf("  %-11s%5zu\n", label, value);
}

/*
 * Prints a full report on the shape and contents of the hash table.
 *
 * The dictionary is only read, never changed. Nothing is allocated in this
 * function, so there is nothing to free; the dictionary keeps owning every
 * entry the walk below passes over. A NULL dictionary and an empty
 * dictionary both print a sensible report instead of crashing or dividing
 * by zero.
 */
void displayStatistics(const Dictionary *dictionary)
{
    size_t partOfSpeechCounts[POS_CATEGORY_COUNT] = {0};
    DictionaryEntry *current = NULL;
    size_t bucketIndex = 0;
    size_t chainLength = 0;
    size_t totalEntries = 0;
    size_t usedBuckets = 0;
    size_t emptyBuckets = 0;
    size_t longestChain = 0;
    double loadFactor = 0.0;
    double averageUsedChain = 0.0;
    int category = 0;

    /* Never follow the pointer before checking that it is real. */
    if (dictionary == NULL)
    {
        printf("Error: There is no dictionary to report on.\n");
        return;
    }

    /* The walk below reads the bucket array, so that pointer is checked too. */
    if (dictionary->buckets == NULL)
    {
        printf("Error: The dictionary has no bucket array to report on.\n");
        return;
    }

    /*
     * One pass over the whole table collects every number in the report:
     * the chain length of each bucket, and the group each word belongs to.
     */
    for (bucketIndex = 0; bucketIndex < dictionary->bucketCount; bucketIndex = bucketIndex + 1)
    {
        chainLength = 0;

        /* current only ever borrows entries the dictionary owns. */
        current = dictionary->buckets[bucketIndex];
        while (current != NULL)
        {
            chainLength = chainLength + 1;

            category = partOfSpeechCategory(current->partOfSpeech);
            partOfSpeechCounts[category] = partOfSpeechCounts[category] + 1;

            /* Step along the chain; no entry is changed or freed here. */
            current = current->next;
        }

        totalEntries = totalEntries + chainLength;

        if (chainLength == 0)
        {
            emptyBuckets = emptyBuckets + 1;
        }
        else
        {
            usedBuckets = usedBuckets + 1;
        }

        if (chainLength > longestChain)
        {
            longestChain = chainLength;
        }
    }

    /* A table with no buckets at all would make this a division by zero. */
    if (dictionary->bucketCount > 0)
    {
        loadFactor = (double)totalEntries / (double)dictionary->bucketCount;
    }

    /*
     * Empty buckets are deliberately left out of this average, so the
     * divisor is the number of used buckets and not the whole table. An
     * empty dictionary has no used buckets, so the average stays at 0.
     */
    if (usedBuckets > 0)
    {
        averageUsedChain = (double)totalEntries / (double)usedBuckets;
    }

    printf("========== Dictionary Statistics ==========\n");
    printCountLine("Total entries:", totalEntries);
    printCountLine("Total buckets:", dictionary->bucketCount);
    printCountLine("Used buckets:", usedBuckets);
    printCountLine("Empty buckets:", emptyBuckets);
    printRatioLine("Load factor:", loadFactor);
    printCountLine("Longest chain:", longestChain);
    printRatioLine("Average used-chain length:", averageUsedChain);
    printf("\n");
    printf("Words by part of speech:\n");
    printPartOfSpeechLine("noun:", partOfSpeechCounts[POS_NOUN]);
    printPartOfSpeechLine("verb:", partOfSpeechCounts[POS_VERB]);
    printPartOfSpeechLine("adjective:", partOfSpeechCounts[POS_ADJECTIVE]);
    printPartOfSpeechLine("adverb:", partOfSpeechCounts[POS_ADVERB]);
    printPartOfSpeechLine("other:", partOfSpeechCounts[POS_OTHER]);
    printf("===========================================\n");
}
