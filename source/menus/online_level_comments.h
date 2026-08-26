#pragma once
#include <3ds.h>

void online_comments_init();
int online_comments_loop();

void online_comments_draw();

extern bool comments_need_refresh;

extern int comments_sort_type;
extern int current_comments_page;
