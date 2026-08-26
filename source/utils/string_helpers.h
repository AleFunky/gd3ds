#pragma once

#include <stdbool.h>

bool contains(const char *first, const char *second);
bool parse_bool(const char *str);
char *truncate_number(int number);
void strip_character(char* s, char character);
char *url_decode(const char *str);
void url_convert_to_http(char *str);