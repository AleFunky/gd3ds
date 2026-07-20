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
extern bool ratedFilter;
extern bool featuredFilter;
extern bool songFilter;
extern bool customSelected;
extern int normalSongId;
extern char songFilterId[127];

void search_filters_init();
int search_filters_loop();