#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

int readLine(char *buffer, size_t bufferSize);
void trimNewline(char *str);
int isBlank(const char *str);
int readMenuChoice(void);

#endif