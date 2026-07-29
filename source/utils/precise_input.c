#include "precise_input.h"
#include <stdio.h>

#define INPUT_QUEUE_SIZE 16
#define RING_BUFFER_ENTRIES 8

bool pi_enabled = false;

static u32 jump_keys;
static bool pressed_edge;

static PreciseInputEvent queue[INPUT_QUEUE_SIZE];
static u32 queue_count;
static u32 queue_head;

static u32 last_idx;
static u32 last_poll_tick;
static bool first_poll = true;

static bool hold_state;
static bool suppress_hold_until_release;

static u32 frame_start;
static u32 frame_end;
static u32 frame_substeps;

static u32 sample_interval(u32 latest_tick, u32 prev_tick);
static u32 reconstruct_tick(u32 newest_time, u32 samples_ago, u32 interval);
static bool ring_lapped(void);
static void push_event(u32 tick, bool down);
static void resync_from_ring(void);
static u32 substep_cutoff(u32 substep);

void pi_reset(void) {
}


// we can read directly from the hid sysmodule (hidSharedMem) to extract 
// the exact time a button was clicked (no syscalls, super fast!): https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0x0
void pi_poll(void) {
    u32 latest_tick = (u32)(*(vu64*)&hidSharedMem[0]);
    u32 prev_tick = (u32)(*(vu64*)&hidSharedMem[2]);
    u32 current_idx = hidSharedMem[4];

    u32 interval = sample_interval(latest_tick, prev_tick);
    if (interval == 0) {
        resync_from_ring();
        return;
    }

    u32 newest_time = latest_tick + (current_idx * interval);

    u32 idx_advanced = (current_idx - last_idx) & 7;

    last_idx = current_idx;

    u32 ring_buffer_offset = 0x28;
    vu32 *pointer_to_ring_buffer = (vu32*)((u8*)hidSharedMem + ring_buffer_offset);

    for(u32 i = idx_advanced; i-- > 0;){
        u32 slot = (current_idx - i) & 7;
        u32 pressed = pointer_to_ring_buffer[1 + (4 * slot)];
        u32 released = pointer_to_ring_buffer[2 + (4 * slot)];
        if(pressed || released){
            u32 input_tick = reconstruct_tick(newest_time, i, interval);
            push_event(input_tick, pressed);
        }
    }
}

void pi_begin_frame(u32 frame_start_tick, u32 frame_end_tick, u32 substeps) {
}

void pi_apply_substep(u32 substep) {
}

static u32 sample_interval(u32 latest_tick, u32 prev_tick) {
    return (latest_tick - prev_tick) / RING_BUFFER_ENTRIES;
}

static u32 reconstruct_tick(u32 newest_time, u32 samples_ago, u32 interval) {
    return newest_time - (samples_ago * interval);
}

static bool ring_lapped(void) {
    return false;
}

static void push_event(u32 tick, bool down) {
    if (queue_count >= INPUT_QUEUE_SIZE) {
        return;
    }
    PreciseInputEvent pie;
    pie.down = down;
    pie.tick = tick;
    queue[(queue_head + queue_count) % INPUT_QUEUE_SIZE] = pie;
    queue_count++;
}

static void resync_from_ring(void) {
}

static u32 substep_cutoff(u32 substep) {
    return 0;
}

void pi_set_jump_keys(u32 mask) {
    jump_keys = mask;
}

void pi_suppress_until_release(void) {
}

bool pi_hold(void) {
    return hold_state;
}

bool pi_pressed(void) {
    return pressed_edge;
}

u32 pi_event_count(void) {
    return queue_count;
}

PreciseInputEvent pi_event_get(u32 index) {
    return queue[(queue_head + index) % INPUT_QUEUE_SIZE];
}
