#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dictionary.h"

int main(void)
{
    printf("Running Prompt 1 Unit Tests (Skeleton & Memory Management)...\n");

    /* 1. Test createDictionary */
    Dictionary *dict = createDictionary(17);
    assert(dict != NULL);
    assert(dict->bucketCount == 17);
    assert(dict->entryCount == 0);
    assert(dict->buckets != NULL);
    for (size_t i = 0; i < dict->bucketCount; i++)
    {
        assert(dict->buckets[i] == NULL);
    }
    printf("[PASS] createDictionary initialization and NULL checks\n");

    /* 2. Test hashWord case-insensitivity */
    size_t h1 = hashWord("Lexicon", 17);
    size_t h2 = hashWord("lexicon", 17);
    size_t h3 = hashWord("LEXICON", 17);
    assert(h1 == h2);
    assert(h2 == h3);
    assert(h1 < 17);
    printf("[PASS] hashWord case-insensitivity\n");

    /* 3. Test duplicateString */
    const char *original = "Safe Memory Allocation";
    char *copy = duplicateString(original);
    assert(copy != NULL);
    assert(strcmp(copy, original) == 0);
    assert(copy != original);
    free(copy);

    assert(duplicateString(NULL) == NULL);
    printf("[PASS] duplicateString\n");

    /* 4. Test destroyDictionary */
    destroyDictionary(dict);
    destroyDictionary(NULL);
    printf("[PASS] destroyDictionary memory cleanup\n");

    printf("\nALL PROMPT 1 TESTS PASSED SUCCESSFULLY!\n");
    return 0;
}
