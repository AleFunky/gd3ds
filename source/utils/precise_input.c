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
    resync_from_ring();
    suppress_hold_until_release = false;
}


// we can read directly from the hid sysmodule (hidSharedMem) to extract 
// the exact time a button was clicked (no syscalls, super fast!): https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0x0
void pi_poll(void) {
    u32 now = (u32)(svcGetSystemTick());
    u32 latest_tick = (u32)(*(vu64*)&hidSharedMem[0]);
    u32 prev_tick = (u32)(*(vu64*)&hidSharedMem[2]);
    u32 current_idx = hidSharedMem[4];

    u32 interval = sample_interval(latest_tick, prev_tick);

    // if lag occurs we resync info from the ring buffer
    if (interval == 0) {
        resync_from_ring();
        return;
    }

    u32 newest_time = latest_tick + (current_idx * interval);

    u32 idx_advanced = (current_idx - last_idx) & 7;

    // calculate the number of samples elapsed since last tick
    u32 elapsed_time = now - last_poll_tick;
    u32 samples_elapsed = (elapsed_time + (interval / 2)) / interval;

    // data was overwritted, resync
    if(samples_elapsed > 8){
        resync_from_ring();
        return;
    }

    // if at 30fps, then idx_advanced == 0 will look the same as moving ahead 8 slots 
    // (making a whole lap around the ring buffer), we must differentiate between idx 
    // not advancing and doing a whole lap by looking at the number of samples elapsed 
    // since the last cpu tick
    if(!idx_advanced && samples_elapsed >= 4){
        idx_advanced = 8;
    }

    u32 ring_buffer_offset = 0x28;
    vu32 *pointer_to_ring_buffer = (vu32*)((u8*)hidSharedMem + ring_buffer_offset);

    for(u32 i = idx_advanced; i-- > 0;){
        u32 slot = (current_idx - i) & 7;
        hold_state = pointer_to_ring_buffer[(sizeof(u32) * slot)] & KEY_A;
        u32 pressed = pointer_to_ring_buffer[1 + (sizeof(u32) * slot)] & KEY_A;
        u32 released = pointer_to_ring_buffer[2 + (sizeof(u32) * slot)] & KEY_A;
        if(pressed || released){
            u32 input_tick = reconstruct_tick(newest_time, i, interval);
            push_event(input_tick, pressed);
        }
    }
    last_idx = current_idx;
    last_poll_tick = now;
}

void pi_begin_frame(u32 frame_start_tick, u32 frame_end_tick, u32 substeps) {
    frame_start = frame_start_tick;
    frame_end = frame_end_tick;
    frame_substeps = (substeps == 0) ? 1 : substeps;
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
    queue_count = 0;
    queue_head = 0;

    last_idx = hidSharedMem[4];

    last_poll_tick = (u32)svcGetSystemTick();
}

static u32 substep_cutoff(u32 substep) {
    return 0;
}

void pi_set_jump_keys(u32 mask) {
    jump_keys = mask;
}

void pi_suppress_until_release(void) {
    suppress_hold_until_release = true;
}

bool pi_hold(void) {
    return hold_state && !suppress_hold_until_release;
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
