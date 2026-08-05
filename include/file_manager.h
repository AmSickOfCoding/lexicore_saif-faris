#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "dictionary.h"

int loadDictionaryFromFile(Dictionary *dictionary, const char *filename);
int saveDictionaryToFile(const Dictionary *dictionary, const char *filename);

#endif