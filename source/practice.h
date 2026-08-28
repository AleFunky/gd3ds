#pragma once

#include <3ds.h>

#define AUTO_CHECKPOINT_TIME 1.f

extern int checkpoint_count;
extern int checkpoint_pointer;
extern bool pseudo_checkpoint_exists;

void start_practice_mode();
void exit_practice_mode();

int get_checkpoint_count();

void new_checkpoint();
void restore_checkpoint();
void delete_last_checkpoint();

void handle_practice_mode();
void draw_checkpoints();
void clear_practice_mode();

void handle_auto_checkpoints(float delta);
void set_checkpoint_timer(float timer);