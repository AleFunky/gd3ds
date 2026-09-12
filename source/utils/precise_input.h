#pragma once
#include <3ds.h>
#include <stdbool.h>

#define PI_PLAYER_COUNT 2

typedef struct {
    u32 tick;
    bool down;
} PreciseInputEvent;

extern bool pi_enabled;

void pi_reset(void);
void pi_poll(void);
void pi_begin_frame(u32 frame_start_tick, u32 frame_end_tick, u32 substeps);
void pi_apply_substep(u32 substep);

void pi_set_jump_keys(u32 player, u32 mask);
void pi_set_touch_filter(u32 player, bool (*filter)(u16 px, u16 py));
void pi_suppress_until_release(void);
bool pi_hold(u32 player);
bool pi_pressed(u32 player);

u32 pi_pad_event_count(u32 player);
PreciseInputEvent pi_pad_event_get(u32 player, u32 index);
u32 pi_touch_event_count(u32 player);
PreciseInputEvent pi_touch_event_get(u32 player, u32 index);
