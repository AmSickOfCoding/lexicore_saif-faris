# Contributions

## Saif (Trainee A)
- Implemented core memory management and dictionary initialization (`duplicateString`, `createDictionary`, `hashWord`, `destroyDictionary`).
- Implemented core dictionary operations (`findWord`, `addWord`, `updateWord`, `deleteWord`) in `src/dictionary.c`.
- Enforced strict direct pointer arithmetic protocol across all hash table bucket traversals.
- Implemented memory safety patterns (allocate-before-free update pattern, duplicate rejection status codes, and zero-leak rollback).
- Authored Doxygen-style documentation across all dictionary module functions.
- Authored the system architecture, memory strategy, and merge conflict resolution specification in `docs/design.md`.

## Faris (Trainee B)
- Implemented safe input validation module (`readLine`, `trimNewline`, `isBlank`, `readMenuChoice`) in `src/input.c`.
- Implemented file storage and streaming loader (`loadDictionaryFromFile`, `saveDictionaryToFile`) in `src/file_manager.c`.
- Implemented statistics module and part-of-speech distribution report in `src/statistics.c`.
- Implemented search and display functions (`findWordsByPrefix`, `displayDictionaryAlphabetically`) in `src/search_display.c`.
- Implemented interactive main menu application loop in `src/main.c`.
- Maintained test suite, sample datasets, and user documentation in `README.md`.

## Joint Work
- Designed the overall hash table architecture and separate chaining collision resolution strategy.
- Formulated the pipe-delimited dictionary file format (`word|part of speech|definition|example sentence`).
- Defined module interfaces, header prototypes, and return code conventions in `include/`.
- Conducted pair programming code reviews and resolved documented merge conflicts in `docs/design.md`.