
#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

// WHY DOESN'T C STANDARD CONTAIN STRCASESTR
bool contains(const char *first, const char *second) {
    if (*second == '\0')
        return true;

    for (; *first; first++) {
        const char *f = first;
        const char *s = second;

        while (*f && *s &&
               tolower((unsigned char)*f) == tolower((unsigned char)*s)) {
            f++;
            s++;
        }

        if (*s == '\0')
            return true;
    }

    return false;
}

bool parse_bool(const char *str) {
    return (str[0] == '1' && str[1] == '\0');
}

// Returns number but only using 3 significant digits, make sure to free this
char *truncate_number(int number) {
    char *buffer = malloc(20);

    if (!buffer) return NULL;

    int positive = abs(number);

    if (positive >= 1000000000) { // Billions (must fry)
        float value = number / 1000000000.f;
        snprintf(buffer, 20, "%.3gB", value);
    } else if (positive >= 1000000) { // Millions
        float value = number / 1000000.f;
        snprintf(buffer, 20, "%.3gM", value);
    } else if (positive >= 1000) { // Thousands
        float value = number / 1000.f;
        snprintf(buffer, 20, "%.3gK", value);
    } else {
        snprintf(buffer, 20, "%d", number);
    }

    return buffer;
}