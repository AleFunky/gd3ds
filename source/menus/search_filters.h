#pragma once

typedef struct {
    const char *chk_name;
    bool *var;
} Filter;

void reset_search_filters();

extern bool uncompletedFilter;
extern bool completedFilter;
extern bool originalFilter;
extern bool unratedFilter;
extern bool featuredFilter;
extern bool songFilter;
extern char songFilterId[8];

void search_filters_init();
int search_filters_loop();