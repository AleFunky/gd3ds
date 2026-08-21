#pragma once

void search_menu_loop();

extern char search_query[129];
extern bool search_needs_refresh;

typedef struct SearchFilters{
    bool uncompleted:1;
    bool completed:1;
    bool original:1;
    bool noStar:1;
    bool star:1;
    bool featured:1;
    bool searchTiny:1;
    bool searchShort:1;
    bool searchMedium:1;
    bool searchLong:1;
    bool searchXl:1;
    bool songFilter:1;
    int searchType;
    int currentPage;
    int songQuery;
    char searchQuery[129];
} SearchFilters;

extern SearchFilters search_filters;
