#pragma once

#include <stdbool.h>
#include <stddef.h>

bool contains(const char *first, const char *second);
bool parseBool(const char *str);
char *truncateNumber(int number);
char *truncateSpeed(size_t bytes);
void stripCharacter(char* s, char character);
void stripEnclosures(char* s);
char *urlDecode(const char *str);
void urlConvertToHTTP(char *str);
void removeChar(char *str, char character);
const char *getFilenameExt(const char *filename);