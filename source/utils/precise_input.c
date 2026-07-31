#include "precise_input.h"

#define INPUT_QUEUE_SIZE 16
#define RING_BUFFER_ENTRIES 8
#define RING_BUFFER_OFFSET 0x28
#define RING_ENTRY_WORDS 4
#define MAX_SAMPLE_INTERVAL_TICKS ((u32)(CPU_TICKS_PER_MSEC * 100))

bool pi_enabled = false;

static u32 jump_keys;
static bool pressed_edge;
static bool last_sample_jump;

static PreciseInputEvent queue[INPUT_QUEUE_SIZE];
static u32 queue_count;
static u32 queue_head;

static u32 last_idx;
static u32 last_poll_tick;

static bool hold_state;
static bool suppress_hold_until_release;
static bool fake_press;

static u32 frame_start;
static u32 frame_end;
static u32 frame_substeps;

static vu32 *get_ring_buffer(void);
static u32 sample_interval(u32 latest_tick, u32 prev_tick);
static u32 reconstruct_tick(u32 newest_time, u32 samples_ago, u32 interval);
static bool push_event(u32 tick, bool down);
static PreciseInputEvent peek_event(void);
static PreciseInputEvent pop_event(void);
static void resync_from_ring(void);
static u32 substep_cutoff(u32 substep);

void pi_set_jump_keys(u32 mask) {
    jump_keys = mask;
}

void pi_reset(void) {
    resync_from_ring();
    suppress_hold_until_release = false;
    fake_press = false;
}

void pi_suppress_until_release(void) {
    suppress_hold_until_release = true;
}

// poll to find the times that the clicks occured and schdule them for later application
void pi_poll(void) {
    u32 now = (u32)(svcGetSystemTick());


    // the hid sysmodule shares its button samples with every process (hidSharedMem),
    // so we can read its ring buffer directly to find when a button was clicked: https://www.3dbrew.org/wiki/HID_Shared_Memory#Offset_0x0
    u32 latest_tick = (u32)(*(vu64*)&hidSharedMem[0]);
    u32 prev_tick = (u32)(*(vu64*)&hidSharedMem[2]);
    u32 current_idx = hidSharedMem[4];

    u32 interval = sample_interval(latest_tick, prev_tick);

    // the lap ticks aren't initialized yet (zero or only one stamp so far), so the
    // interval is zero or its huge (garbage) and we can't reconstruct timestamps, we resync instead
    if (interval == 0 || interval > MAX_SAMPLE_INTERVAL_TICKS) {
        resync_from_ring();
        return;
    }

    u32 newest_time = latest_tick + (current_idx * interval);

    u32 idx_advanced = (current_idx - last_idx) & (RING_BUFFER_ENTRIES - 1);

    // calculate the number of samples elapsed since last poll
    u32 elapsed_time = now - last_poll_tick;
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

    vu32 *pointer_to_ring_buffer = get_ring_buffer();

    for(u32 i = idx_advanced; i-- > 0;){
        u32 slot = (current_idx - i) & (RING_BUFFER_ENTRIES - 1);
        bool jump = (pointer_to_ring_buffer[RING_ENTRY_WORDS * slot] & jump_keys) != 0;
        if(jump != last_sample_jump){
            u32 input_tick = reconstruct_tick(newest_time, i, interval);
            if(!push_event(input_tick, jump)){
                resync_from_ring();
                return;
            }
        }
        last_sample_jump = jump;
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
    pressed_edge = fake_press;
    fake_press = false;
    u32 cutoff = substep_cutoff(substep);
    bool final_substep = (substep + 1 >= frame_substeps);
    bool pressed_this_substep = false;

    while(queue_count > 0){
        PreciseInputEvent pie = peek_event();

        // stop at the first event that isnt due yet except on the final substep, which must flush everything.
        if(!final_substep && (s32)(pie.tick - cutoff) > 0){
            break;
        }
        if(!final_substep && pressed_this_substep){
            break;
        }

        pop_event();

        if(pie.down){
            hold_state = true;
            pressed_edge = true;
            pressed_this_substep = true;
        }else{
            hold_state =false;
            suppress_hold_until_release = false;
        }
    }
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

static vu32 *get_ring_buffer(void) {
    return (vu32*)((u8*)hidSharedMem + RING_BUFFER_OFFSET);
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

static bool push_event(u32 tick, bool down) {
    if (queue_count >= INPUT_QUEUE_SIZE) {
        return false;
    }
    PreciseInputEvent pie;
    pie.down = down;
    pie.tick = tick;
    queue[(queue_head + queue_count) % INPUT_QUEUE_SIZE] = pie;
    queue_count++;
    return true;
}

static PreciseInputEvent peek_event(void) {
    return queue[queue_head];
}

static PreciseInputEvent pop_event(void) {
    PreciseInputEvent pie = queue[queue_head];
    queue_head = (queue_head + 1) % INPUT_QUEUE_SIZE;
    queue_count--;
    return pie;
}

static void resync_from_ring(void) {
    bool was_holding = hold_state;

    queue_count = 0;
    queue_head = 0;

    last_idx = hidSharedMem[4];

    vu32 *pointer_to_ring_buffer = get_ring_buffer();
    hold_state = (pointer_to_ring_buffer[RING_ENTRY_WORDS * last_idx] & jump_keys) != 0;   // is a jump key down NOW?
    last_sample_jump = hold_state;

    if(!hold_state){
        suppress_hold_until_release = false;
    }

    // the press edge was overwritten by the lap, but the state change survived.
    // was can create the edge from the evidencewe have. This makes the ufo work a low frame rates
    // because ufo's require an edge to avoid continuous flapping while holding the button.
    if(!was_holding && hold_state && !suppress_hold_until_release){
        fake_press = true;
    }

    last_poll_tick = (u32)svcGetSystemTick();
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
