/*
 * statistics.c - The dictionary statistics report for LexiCore.
 *
 * Every number printed here is worked out by walking the hash table at the
 * moment the report is asked for. Nothing is stored between calls and no
 * running total is kept inside the Dictionary struct, so the figures can
 * never drift out of step with what the table actually holds.
 */

#include "statistics.h"

#include <stdio.h>

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
    DictionaryEntry *current = NULL;
    size_t bucketIndex = 0;
    size_t chainLength = 0;
    size_t totalEntries = 0;
    size_t usedBuckets = 0;
    size_t emptyBuckets = 0;
    size_t longestChain = 0;
    double loadFactor = 0.0;
    double averageUsedChain = 0.0;

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
     * One pass over the whole table measures the chain length of every
     * bucket, which is where all the numbers below come from.
     */
    for (bucketIndex = 0; bucketIndex < dictionary->bucketCount; bucketIndex = bucketIndex + 1)
    {
        chainLength = 0;

        /* current only ever borrows entries the dictionary owns. */
        current = dictionary->buckets[bucketIndex];
        while (current != NULL)
        {
            chainLength = chainLength + 1;

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
    printf("===========================================\n");
}
