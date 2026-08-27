#pragma once

#include <stdbool.h>
#include <stddef.h>

bool contains(const char *first, const char *second);
bool parse_bool(const char *str);
char *truncate_number(int number);
char *truncate_speed(size_t bytes);
void strip_character(char* s, char character);
char *url_decode(const char *str);
void url_convert_to_http(char *str);