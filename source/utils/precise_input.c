#include "precise_input.h"

#define INPUT_QUEUE_SIZE 16
#define RING_BUFFER_ENTRIES 8
#define RING_ENTRY_WORDS 4
#define TOUCH_RING_ENTRY_WORDS 2
#define MAX_SAMPLE_INTERVAL_TICKS ((u32)(CPU_TICKS_PER_MSEC * 100))

bool pi_enabled = false;

static u32 jump_keys;
static bool (*touch_filter)(u16 px, u16 py);

static u32 frame_start;
static u32 frame_end;
static u32 frame_substeps;

typedef struct {
    u32 tick_word;
    u32 idx_word;

    u32 ring_buffer_offset;

    bool (*sample_jump)(u32 slot);

    PreciseInputEvent queue[INPUT_QUEUE_SIZE];
    u32 queue_count;
    u32 queue_head;

    u32 last_idx;
    u32 last_poll_tick;
    bool last_sample_jump;

    bool hold_state;
    bool suppress_hold_until_release;
    bool fake_press;
    bool pressed_edge;
} PreciseSource;

static bool pad_sample_jump(u32 slot);
static bool touch_sample_jump(u32 slot);

// https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0x0
static PreciseSource pad = {
    .tick_word = 0,
    .idx_word = 4,
    .ring_buffer_offset = 0x28,
    .sample_jump = pad_sample_jump,
};

// https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0xA8
static PreciseSource touch = {
    .tick_word = 42,
    .idx_word = 46,
    .ring_buffer_offset = 0xA8 + 0x20,
    .sample_jump = touch_sample_jump,
};

static vu32 *get_ring_buffer(PreciseSource* src);
static u32 sample_interval(u32 latest_tick, u32 prev_tick);
static u32 reconstruct_tick(u32 newest_time, u32 samples_ago, u32 interval);
static bool push_event(PreciseSource *src, u32 tick, bool down);
static PreciseInputEvent peek_event(PreciseSource *src);
static PreciseInputEvent pop_event(PreciseSource *src);
static void poll_source(PreciseSource *src);
static void apply_source(PreciseSource *src, u32 substep);
static void resync_from_ring(PreciseSource *src);
static u32 substep_cutoff(u32 substep);

void pi_set_jump_keys(u32 mask) {
    jump_keys = mask;
}

void pi_reset(void) {
    resync_from_ring(&pad);
    pad.suppress_hold_until_release = false;
    pad.fake_press = false;
}

void pi_suppress_until_release(void) {
    pad.suppress_hold_until_release = true;
}

// poll to find the times that the clicks occured and schdule them for later application
void pi_poll(void) {
    poll_source(&pad);
}

void pi_begin_frame(u32 frame_start_tick, u32 frame_end_tick, u32 substeps) {
    frame_start = frame_start_tick;
    frame_end = frame_end_tick;
    frame_substeps = (substeps == 0) ? 1 : substeps;
}

void pi_apply_substep(u32 substep) {
    apply_source(&pad, substep);
}

bool pi_hold(void) {
    return pad.hold_state && !pad.suppress_hold_until_release;
}

bool pi_pressed(void) {
    return pad.pressed_edge;
}

u32 pi_event_count(void) {
    return pad.queue_count;
}

PreciseInputEvent pi_event_get(u32 index) {
    return pad.queue[(pad.queue_head + index) % INPUT_QUEUE_SIZE];
}

static bool pad_sample_jump(u32 slot) {
    vu32 *pointer_to_ring_buffer = get_ring_buffer(&pad);
    return (pointer_to_ring_buffer[RING_ENTRY_WORDS * slot] & jump_keys) != 0;
}

static bool touch_sample_jump(u32 slot) {
    vu32 *pointer_to_ring_buffer = get_ring_buffer(&touch);
    u32 position = pointer_to_ring_buffer[TOUCH_RING_ENTRY_WORDS * slot];
    u32 valid = pointer_to_ring_buffer[TOUCH_RING_ENTRY_WORDS * slot + 1];

    if (!valid) {
        return false;
    }

    if (touch_filter) {
        u16 px = (u16)(position & 0xFFFF);
        u16 py = (u16)(position >> 16);
        return touch_filter(px, py);
    }

    return true;
}

static vu32 *get_ring_buffer(PreciseSource* src) {
    return (vu32*)((u8*)hidSharedMem + src->ring_buffer_offset);
}

// calculates the duration between HID samples
static u32 sample_interval(u32 latest_tick, u32 prev_tick) {
    return (latest_tick - prev_tick) / RING_BUFFER_ENTRIES;
}

// calculates the estimated (+/- interval) time the click occured. assumes click/release has occured
static u32 reconstruct_tick(u32 newest_time, u32 samples_ago, u32 interval) {
    // get the newest time and subtract how long ago the click happened
    return newest_time - (samples_ago * interval);
}

static bool push_event(PreciseSource *src, u32 tick, bool down) {
    if (src->queue_count >= INPUT_QUEUE_SIZE) {
        return false;
    }
    PreciseInputEvent pie;
    pie.down = down;
    pie.tick = tick;
    src->queue[(src->queue_head + src->queue_count) % INPUT_QUEUE_SIZE] = pie;
    src->queue_count++;
    return true;
}

static PreciseInputEvent peek_event(PreciseSource *src) {
    return src->queue[src->queue_head];
}

static PreciseInputEvent pop_event(PreciseSource *src) {
    PreciseInputEvent pie = src->queue[src->queue_head];
    src->queue_head = (src->queue_head + 1) % INPUT_QUEUE_SIZE;
    src->queue_count--;
    return pie;
}

static void poll_source(PreciseSource *src) {
    u32 now = (u32)(svcGetSystemTick());


    // the hid sysmodule shares its button samples with every process (hidSharedMem),
    // so we can read its ring buffer directly to find when a button was clicked: https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0x0
    u32 latest_tick = (u32)(*(vu64*)&hidSharedMem[src->tick_word]);
    u32 prev_tick = (u32)(*(vu64*)&hidSharedMem[src->tick_word + 2]);
    u32 current_idx = hidSharedMem[src->idx_word];

    u32 interval = sample_interval(latest_tick, prev_tick);

    // the lap ticks aren't initialized yet (zero or only one stamp so far), so the
    // interval is zero or its huge (garbage) and we can't reconstruct timestamps, we resync instead
    if (interval == 0 || interval > MAX_SAMPLE_INTERVAL_TICKS) {
        resync_from_ring(src);
        return;
    }

    u32 newest_time = latest_tick + (current_idx * interval);

    u32 idx_advanced = (current_idx - src->last_idx) & (RING_BUFFER_ENTRIES - 1);

    // calculate the number of samples elapsed since last poll
    u32 elapsed_time = now - src->last_poll_tick;
    u32 samples_elapsed = (elapsed_time + (interval / 2)) / interval;

    // data was overwritten, but insted of resyncing immediatly we go through the survivors
    // for any recent potential inputs
    if(samples_elapsed > RING_BUFFER_ENTRIES){
        idx_advanced = RING_BUFFER_ENTRIES;
    }

    // if at 30fps, then idx_advanced == 0 will look the same as moving ahead 8 slots
    // (making a whole lap around the ring buffer), we must differenciate between idx
    // not advancing and doing a whole lap by looking at the number of samples elapsed
    // since the last cpu tick
    if(!idx_advanced && samples_elapsed >= (RING_BUFFER_ENTRIES / 2)){
        idx_advanced = RING_BUFFER_ENTRIES;
    }

    for(u32 i = idx_advanced; i-- > 0;){
        u32 slot = (current_idx - i) & (RING_BUFFER_ENTRIES - 1);
        bool jump = src->sample_jump(slot);
        if(jump != src->last_sample_jump){
            u32 input_tick = reconstruct_tick(newest_time, i, interval);
            if(!push_event(src, input_tick, jump)){
                resync_from_ring(src);
                return;
            }
        }
        src->last_sample_jump = jump;
    }
    src->last_idx = current_idx;
    src->last_poll_tick = now;
}

static void apply_source(PreciseSource *src, u32 substep) {
    src->pressed_edge = src->fake_press;
    src->fake_press = false;
    u32 cutoff = substep_cutoff(substep);
    bool final_substep = (substep + 1 >= frame_substeps);
    bool pressed_this_substep = false;

    while(src->queue_count > 0){
        PreciseInputEvent pie = peek_event(src);

        // stop at the first event that isnt due yet except on the final substep, which must flush everything.
        if(!final_substep && (s32)(pie.tick - cutoff) > 0){
            break;
        }
        if(!final_substep && pressed_this_substep){
            break;
        }

        pop_event(src);

        src->suppress_hold_until_release = false;

        if(pie.down){
            src->hold_state = true;
            src->pressed_edge = true;
            pressed_this_substep = true;
        }else{
            src->hold_state = false;
        }
    }
}

static void resync_from_ring(PreciseSource *src) {
    bool was_holding = src->hold_state;

    src->queue_count = 0;
    src->queue_head = 0;

    src->last_idx = hidSharedMem[src->idx_word];

    src->hold_state = src->sample_jump(src->last_idx);   // is a jump key down NOW?
    src->last_sample_jump = src->hold_state;

    if(!src->hold_state){
        src->suppress_hold_until_release = false;
    }

    // the press edge was overwritten by the lap, but the state change survived.
    // was can create the edge from the evidencewe have. This makes the ufo work a low frame rates
    // because ufo's require an edge to avoid continuous flapping while holding the button.
    if(!was_holding && src->hold_state && !src->suppress_hold_until_release){
        src->fake_press = true;
    }

    src->last_poll_tick = (u32)svcGetSystemTick();
}

// calculates the tick where substeps slice of the frame window ends (events are due at or before it)
static u32 substep_cutoff(u32 substep) {
    u32 span = frame_end - frame_start;
    u32 slices_done = substep + 1;

    // on a lag spike the window can reach 0.5s with slices_done up to ~120. that
    // product overflows if the multiplication happens in u32, so we widen to u64 first
    u64 scaled_span = (u64)span * slices_done;
    u32 offset_into_frame = (u32)(scaled_span / frame_substeps);
    return frame_start + offset_into_frame;
}
