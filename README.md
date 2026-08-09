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
## Hash function
## Folder structure
## Compilation and running
## Dictionary file format
## Example usage
## Memory management
## Known limitations
## Contributions