#pragma once
#include <3ds.h>

typedef struct {
    bool copied;
    const char *text;
    bool customTitle;
    const char *title;
} InfoCardData;

extern const UIScreenDefPair info_card_def;