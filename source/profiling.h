#pragma once
#include <3ds.h>

typedef struct {
    touchPosition touchPos;
    u32 kDown;
    int steps;
    float processingTime;
} ProfilerUpdateData;

typedef struct {
    // General
    float frame_ms;
    float cpu_ms;
    float gpu_ms;

    // Timers
    float physics_ms;
    float triggers_ms;
    float collision_ms;
    float particles_ms;
    float rendering_ms;

    // Physics
    float coll_ms;
    float play_ms;
    float handler_ms;
    int steps;
    int collisions;
    int collision_checks;

    // Rendering
    float creating_ms;
    float sorting_ms;
    float tint_ms;
    float drawing_ms;

    // Dirty system
    int draw_dirty;
    int draw_count;

    // Touch
    int input_x;
    int input_y;

    // Player
    int tick;
    float ply_pos_x;
    float ply_pos_y;
    float ply_vel_x;
    float ply_vel_y;

    // Camera
    float cam_pos_x;
    float cam_pos_y;
    float cam_itd_y;

    // Heap
    u32 linear_free;
    u32 heap_used;
    u32 heap_free;
    u32 heap_total;
} ProfilerSnapshot;

extern ProfilerSnapshot snapshot;

void profiler_update(ProfilerUpdateData data);
void profiler_draw();