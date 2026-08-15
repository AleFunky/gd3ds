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
extern bool song_filter_enabled;
extern bool length_filter_enabled;
extern bool custom_song;
extern int normal_song_id_selected;
extern char custom_song_id[127];

void search_filters_init();
int search_filters_loop();

void search_filters_draw();