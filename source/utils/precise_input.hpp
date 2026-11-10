#pragma once
#include <3ds.h>
#include <stdbool.h>

typedef enum {
    PI_PLAYER_1,
    PI_PLAYER_2,
    PI_PLAYER_COUNT
} PreciseInputPlayer;

typedef struct {
    u32 tick;
    bool down;
} PreciseInputEvent;

extern bool pi_enabled;

void pi_reset(void);
void pi_poll(void);
void pi_begin_frame(u32 frame_start_tick, u32 frame_end_tick, u32 substeps);
void pi_apply_substep(u32 substep);

void pi_set_jump_keys(PreciseInputPlayer player, u32 mask);
void pi_set_touch_filter(PreciseInputPlayer player, bool (*filter)(u16 px, u16 py));
void pi_suppress_until_release(void);
bool pi_hold(PreciseInputPlayer player);
bool pi_pressed(PreciseInputPlayer player);

u32 pi_pad_event_count(PreciseInputPlayer player);
PreciseInputEvent pi_pad_event_get(PreciseInputPlayer player, u32 index);
u32 pi_touch_event_count(PreciseInputPlayer player);
PreciseInputEvent pi_touch_event_get(PreciseInputPlayer player, u32 index);
