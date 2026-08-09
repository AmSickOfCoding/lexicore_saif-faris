# LexiCore
**Trainees:** <saif>, <faris>

## Description and features

LexiCore is a command line dictionary written in C11. Words are held in a hash
table while the program runs, stored on disk as plain text, and managed through
a numbered menu. It was written as a two person training project, so the code
favours being readable and safe over being clever.

Features:

- **Search** for a word by its exact spelling. Upper and lower case do not
  matter, so `program`, `Program` and `PROGRAM` all find the same entry.
- **Add** a word with its part of speech, definition and example sentence. A
  word already in the dictionary is refused rather than stored twice.
- **Edit** any field of an existing word, one field at a time.
- **Delete** a word, after a confirmation prompt.
- **Prefix search** lists every word beginning with the letters you type.
- **Alphabetical listing** of the whole dictionary, twenty words to a page.
- **Statistics** on the shape of the hash table and on how many words fall into
  each part of speech.
- **Load and save** pipe separated files. A malformed line is reported and
  skipped rather than stopping the load.
- **Safe input** throughout: no buffer can overflow, `gets()` is never used, and
  a line too long for a buffer is discarded instead of being read as two lines.

## Architecture: hash table and linked lists

The dictionary is one array of buckets. Each bucket holds the head of a singly
linked list, and every word that hashes to that bucket is a node on that list.
This is **separate chaining**: collisions are not avoided, they are simply
chained together.

```
Dictionary
  bucketCount = 503
  entryCount  = 67
  buckets ──> [  0 ] ── NULL
              [  1 ] ── NULL
              [ ... ]
              [ 498 ] ──> "ribbon" ──> "protect" ──> "hesitate" ── NULL
              [ ... ]
              [ 502 ] ── NULL

Each node:  word | partOfSpeech | definition | exampleSentence | next
```

Bucket 498 above is real: with the 503 buckets the program uses, those three
words from `data/dictionary.txt` genuinely collide. New entries are pushed onto
the **front** of the chain, which is why `ribbon`, loaded last of the three,
ends up first.

| Operation | Cost | Why |
|---|---|---|
| Search, add, delete one word | O(1) on average | The hash goes straight to one bucket, and chains stay short while the load factor is low |
| Prefix search | O(n) | A prefix does not hash to a bucket, so every bucket must be walked |
| Alphabetical listing | O(n log n) | Entries are collected into a temporary array and sorted with `qsort` |
| Statistics | O(n) | One pass over the whole table, counting as it goes |

The bucket count is 503, a prime number. A prime is used because the hash is
reduced with `% bucketCount`, and a prime divisor spreads remainders more evenly
than a round number would when the input has repeating patterns.

## Hash function

LexiCore uses the DJB2 hash, in `hashWord`:

```c
unsigned long hash = 5381;
while ((c = (unsigned char)*word++) != '\0')
{
    hash = hash * 33 + (unsigned long)tolower(c);
}
return (size_t)(hash % bucketCount);
```

- It starts from the seed `5381` and, for each character, multiplies the running
  value by `33` and adds the character.
- `tolower` is applied to every character, so `Program` and `PROGRAM` produce the
  same number and therefore land in the same bucket. That is what makes every
  lookup in the program case-insensitive.
- The character is cast to `unsigned char` first, because `tolower` is only
  defined for values that fit in an unsigned char or `EOF`.
- `% bucketCount` folds the result into a real bucket index.

Multiplying by 33 and adding is cheap, and mixing every character into the total
means two words that differ by a single letter usually end up far apart.

How well it spreads the shipped data, as reported by menu option 7:

| Measure | Value |
|---|---|
| Words | 67 |
| Buckets | 503 |
| Used buckets | 62 |
| Load factor | 0.13 |
| Longest chain | 3 |
| Average chain length, counting used buckets only | 1.08 |

## Folder structure
## Compilation and running
## Dictionary file format
## Example usage
## Memory management
## Known limitations
## Contributions