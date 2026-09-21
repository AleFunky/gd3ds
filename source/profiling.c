#include "profiling.h"
#include "3ds/services/hid.h"
#include "c2d/base.h"
#include "c3d/renderqueue.h"
#include "fonts/chatFont.h"
#include "main.h"
#include "math_helpers.h"
#include "menus/core/ui_screen.h"
#include "state.h"

ProfilerSnapshot snapshot;

typedef enum {
    PROFILER_PAGE_OVERVIEW,
    PROFILER_PAGE_RENDER,
    PROFILER_PAGE_GAMEPLAY,
    PROFILER_PAGE_PLAYER_CAMERA,
    PROFILER_PAGE_MISC,
    PROFILER_PAGE_MEMORY,
    PROFILER_PAGE_COUNT
} ProfilerPage;

static ProfilerPage current_page = PROFILER_PAGE_OVERVIEW;

#define PROFILER_SCALE 0.8f
#define PROFILER_ROW_HEIGHT 12.f
#define PROFILER_LABEL_X 0.f
#define PROFILER_VALUE_X 180.f
#define PROFILER_SEC_VALUE_X 260.f
#define PROFILER_START_Y 20.f

typedef enum {
    PROFILER_VALUE_NONE,
    PROFILER_VALUE_FLOAT,
    PROFILER_VALUE_FPS,
    PROFILER_VALUE_MS_PERCENTAGE,
    PROFILER_VALUE_INTEGER,
    PROFILER_VALUE_HEX
} ProfilerValueType;

typedef enum {
    PROFILER_STAT_NONE,

    PROFILER_STAT_FRAME,
    PROFILER_STAT_FPS,
    PROFILER_STAT_CPU,
    PROFILER_STAT_GPU,

    PROFILER_STAT_PHYSICS,
    PROFILER_STAT_TRIGGERS,
    PROFILER_STAT_COLLISION,
    PROFILER_STAT_PARTICLES,
    PROFILER_STAT_RENDERING,
    
    PROFILER_STAT_PLAYER,
    PROFILER_STAT_PLAYER_COLLISION,
    PROFILER_STAT_PLAYER_HANDLER,
    PROFILER_STAT_PLAYER_STEPS,
    PROFILER_STAT_PLAYER_COLLISIONS,
    PROFILER_STAT_PLAYER_COLLISION_CHECKS,

    PROFILER_STAT_RENDER_CREATING,
    PROFILER_STAT_RENDER_SORTING,
    PROFILER_STAT_RENDER_TINT,
    PROFILER_STAT_RENDER_DRAWING,

    PROFILER_STAT_OPTIM_DIRTY,
    PROFILER_STAT_OPTIM_COUNT,
    PROFILER_STAT_OPTIM_IN_CACHE,

    PROFILER_STAT_INPUT_X,
    PROFILER_STAT_INPUT_Y,

    PROFILER_STAT_PLAYER_TICK,
    PROFILER_STAT_PLAYER_X,
    PROFILER_STAT_PLAYER_Y,
    PROFILER_STAT_PLAYER_VX,
    PROFILER_STAT_PLAYER_VY,

    PROFILER_STAT_CAMERA_X,
    PROFILER_STAT_CAMERA_Y,
    PROFILER_STAT_CAMERA_INTENDED_Y,

    PROFILER_STAT_LINEAR_FREE,
    PROFILER_STAT_HEAP_USED,
    PROFILER_STAT_HEAP_FREE,
    PROFILER_STAT_HEAP_TOTAL,
} ProfilerStat;

typedef struct {
    const char *label;
    ProfilerStat stat;
    ProfilerValueType value_type;
} ProfilerRow;

static const ProfilerRow overview_rows[] = {
    { "FPS",       PROFILER_STAT_FPS,       PROFILER_VALUE_FPS },
    { "CPU",       PROFILER_STAT_CPU,       PROFILER_VALUE_MS_PERCENTAGE },
    { "GPU",       PROFILER_STAT_GPU,       PROFILER_VALUE_MS_PERCENTAGE },
    {},
    { "Rendering", PROFILER_STAT_RENDERING, PROFILER_VALUE_MS_PERCENTAGE },
    { "Physics",   PROFILER_STAT_PHYSICS,   PROFILER_VALUE_MS_PERCENTAGE },
};

static const ProfilerRow render_rows[] = {
    { "CPU",        PROFILER_STAT_CPU,             PROFILER_VALUE_MS_PERCENTAGE },
    { "GPU",        PROFILER_STAT_GPU,             PROFILER_VALUE_MS_PERCENTAGE },
    {},
    { "Rendering",  PROFILER_STAT_RENDERING,       PROFILER_VALUE_MS_PERCENTAGE },
    { "- Creating", PROFILER_STAT_RENDER_CREATING, PROFILER_VALUE_MS_PERCENTAGE },
    { "- Sorting",  PROFILER_STAT_RENDER_SORTING,  PROFILER_VALUE_MS_PERCENTAGE },
    { "- Tinting",  PROFILER_STAT_RENDER_TINT,     PROFILER_VALUE_MS_PERCENTAGE },
    { "- Drawing",  PROFILER_STAT_RENDER_DRAWING,  PROFILER_VALUE_MS_PERCENTAGE },
    {},
    { "Particles",  PROFILER_STAT_PARTICLES,       PROFILER_VALUE_MS_PERCENTAGE },
    {},
    { "Optim" },
    { "- Dirty",    PROFILER_STAT_OPTIM_DIRTY,    PROFILER_VALUE_INTEGER},
    { "- Total",    PROFILER_STAT_OPTIM_COUNT,    PROFILER_VALUE_INTEGER},
};

static const ProfilerRow gameplay_rows[] = {
    { "Collisions", PROFILER_STAT_PLAYER_COLLISIONS,       PROFILER_VALUE_INTEGER },
    { "Checks",     PROFILER_STAT_PLAYER_COLLISION_CHECKS, PROFILER_VALUE_INTEGER },
    { "Steps",      PROFILER_STAT_PLAYER_STEPS,            PROFILER_VALUE_INTEGER },
    {},
    { "Physics",     PROFILER_STAT_PHYSICS,          PROFILER_VALUE_MS_PERCENTAGE },
    { "- Player",    PROFILER_STAT_PLAYER,           PROFILER_VALUE_MS_PERCENTAGE },
    { "- Collision", PROFILER_STAT_PLAYER_COLLISION, PROFILER_VALUE_MS_PERCENTAGE },
    { "- Handler",   PROFILER_STAT_PLAYER_HANDLER,   PROFILER_VALUE_MS_PERCENTAGE }
};

static const ProfilerRow player_camera_rows[] = {
    { "Player" },
    { "- X",    PROFILER_STAT_PLAYER_X,  PROFILER_VALUE_FLOAT },
    { "- Y",    PROFILER_STAT_PLAYER_Y,  PROFILER_VALUE_FLOAT },
    { "- VX",   PROFILER_STAT_PLAYER_VX, PROFILER_VALUE_FLOAT },
    { "- VY",   PROFILER_STAT_PLAYER_VY, PROFILER_VALUE_FLOAT },
    {},
    { "Camera" },
    { "- X",          PROFILER_STAT_CAMERA_X,          PROFILER_VALUE_FLOAT },
    { "- Y",          PROFILER_STAT_CAMERA_Y,          PROFILER_VALUE_FLOAT },
    { "- Intended Y", PROFILER_STAT_CAMERA_INTENDED_Y, PROFILER_VALUE_FLOAT }
};

static const ProfilerRow misc_rows[] = {
    { "Input" },
    { "- X",   PROFILER_STAT_INPUT_X, PROFILER_VALUE_INTEGER },
    { "- Y",   PROFILER_STAT_INPUT_Y, PROFILER_VALUE_INTEGER }
};

static const ProfilerRow memory_rows[] = {
    { "Linear free", PROFILER_STAT_LINEAR_FREE, PROFILER_VALUE_HEX },
    {},
    { "Heap used",   PROFILER_STAT_HEAP_USED,   PROFILER_VALUE_HEX },
    { "Heap free",   PROFILER_STAT_HEAP_FREE,   PROFILER_VALUE_HEX },
    { "Heap total",  PROFILER_STAT_HEAP_TOTAL,  PROFILER_VALUE_HEX }
};

static const char *page_names[PROFILER_PAGE_COUNT] = {
    "Overview",
    "Render",
    "Gameplay",
    "Player/Camera",
    "Miscellaneous",
    "Memory",
};

static void profiler_handle_input(u32 kDown) {
    if (kDown & KEY_LEFT) {
        if (current_page == PROFILER_PAGE_OVERVIEW) current_page = PROFILER_PAGE_COUNT - 1;
        else current_page--;
    }

    if (kDown & KEY_RIGHT) {
        current_page = (current_page + 1) % PROFILER_PAGE_COUNT;
    }
}

static void draw_page_header(void) {
    draw_text(
        &chatFont_fontCharset,
        &chatFont_sheet,
        0, 5,
        PROFILER_SCALE, PROFILER_SCALE,
        0,
        true,
        "[%d/%d] %s",
        current_page + 1,
        PROFILER_PAGE_COUNT,
        page_names[current_page]
    );
}

static float profiler_stat_value(ProfilerStat stat) {
    switch (stat) {
        case PROFILER_STAT_FRAME: return snapshot.frame_ms;
        case PROFILER_STAT_FPS: return snapshot.frame_ms > 0 ? 1000.f / snapshot.frame_ms : 0.f;
        case PROFILER_STAT_CPU: return snapshot.cpu_ms;
        case PROFILER_STAT_GPU: return snapshot.gpu_ms;

        case PROFILER_STAT_PHYSICS: return snapshot.physics_ms;
        case PROFILER_STAT_TRIGGERS: return snapshot.triggers_ms;
        case PROFILER_STAT_COLLISION: return snapshot.collision_ms;
        case PROFILER_STAT_PARTICLES: return snapshot.particles_ms;
        case PROFILER_STAT_RENDERING: return snapshot.rendering_ms;

        case PROFILER_STAT_PLAYER: return snapshot.play_ms;
        case PROFILER_STAT_PLAYER_COLLISION: return snapshot.collision_ms;
        case PROFILER_STAT_PLAYER_HANDLER: return snapshot.handler_ms;

        case PROFILER_STAT_RENDER_CREATING: return snapshot.creating_ms;
        case PROFILER_STAT_RENDER_SORTING: return snapshot.sorting_ms;
        case PROFILER_STAT_RENDER_TINT: return snapshot.tint_ms;
        case PROFILER_STAT_RENDER_DRAWING: return snapshot.drawing_ms;

        case PROFILER_STAT_PLAYER_X: return snapshot.ply_pos_x;
        case PROFILER_STAT_PLAYER_Y: return snapshot.ply_pos_y;
        case PROFILER_STAT_PLAYER_VX: return snapshot.ply_vel_x;
        case PROFILER_STAT_PLAYER_VY: return snapshot.ply_vel_y;
        
        case PROFILER_STAT_CAMERA_X: return snapshot.cam_pos_x;
        case PROFILER_STAT_CAMERA_Y: return snapshot.cam_pos_y;
        case PROFILER_STAT_CAMERA_INTENDED_Y: return snapshot.cam_itd_y;

        default: return 0.f;
    }
}

static unsigned int profiler_stat_integer(ProfilerStat stat) {
    switch (stat) {
        case PROFILER_STAT_PLAYER_STEPS: return snapshot.steps;
        case PROFILER_STAT_PLAYER_COLLISIONS: return snapshot.collisions;
        case PROFILER_STAT_PLAYER_COLLISION_CHECKS: return snapshot.collision_checks;

        case PROFILER_STAT_OPTIM_DIRTY: return snapshot.draw_dirty;
        case PROFILER_STAT_OPTIM_COUNT: return snapshot.draw_count;

        case PROFILER_STAT_INPUT_X: return snapshot.input_x;
        case PROFILER_STAT_INPUT_Y: return snapshot.input_y;

        case PROFILER_STAT_PLAYER_TICK: return snapshot.tick;

        case PROFILER_STAT_LINEAR_FREE: return snapshot.linear_free;
        case PROFILER_STAT_HEAP_USED: return snapshot.heap_used;
        case PROFILER_STAT_HEAP_FREE: return snapshot.heap_free;
        case PROFILER_STAT_HEAP_TOTAL: return snapshot.heap_total;
        default: return 0;
    }
}

static void draw_stat_value(const ProfilerRow *row, float y) {
    switch (row->value_type) {
        case PROFILER_VALUE_FLOAT:
            draw_text(&chatFont_fontCharset, &chatFont_sheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.2f",
                profiler_stat_value(row->stat));
            break;
        case PROFILER_VALUE_FPS:
            draw_text(&chatFont_fontCharset, &chatFont_sheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.1f FPS",
                profiler_stat_value(row->stat));
            break;
        case PROFILER_VALUE_MS_PERCENTAGE:
            draw_text(&chatFont_fontCharset, &chatFont_sheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.2f%%",
                profiler_stat_value(row->stat) * 6.f);
            break;
        case PROFILER_VALUE_INTEGER:
            draw_text(&chatFont_fontCharset, &chatFont_sheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%u",
                profiler_stat_integer(row->stat));
            break;
        case PROFILER_VALUE_HEX:
            draw_text(&chatFont_fontCharset, &chatFont_sheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "0x%X",
                profiler_stat_integer(row->stat));
            break;
        default: 
            break;
    }
}

static void draw_secondary_stat_value(const ProfilerRow *row, float y) {
    switch (row->value_type) {
        case PROFILER_VALUE_MS_PERCENTAGE:
            draw_text(&chatFont_fontCharset, &chatFont_sheet, PROFILER_SEC_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "(%.2f ms)",
                profiler_stat_value(row->stat));
            break;
        default:
            break;
    }
}

static void draw_stat_table(const ProfilerRow *rows, unsigned int row_count) {
    for (int row_index = 0; row_index < row_count; row_index++) {
        float y = PROFILER_START_Y + row_index * PROFILER_ROW_HEIGHT;

        // Skip empty
        if (rows[row_index].label == NULL) {
            continue;
        }

        draw_text(&chatFont_fontCharset, &chatFont_sheet, PROFILER_LABEL_X, y,
            PROFILER_SCALE, PROFILER_SCALE, 0, true, "%s", rows[row_index].label);
        draw_stat_value(&rows[row_index], y);
        draw_secondary_stat_value(&rows[row_index], y);
    }
}

static void draw_overview_page() {
    draw_stat_table(overview_rows, ARRAY_LEN(overview_rows));
}

static void draw_render_page() {
    draw_stat_table(render_rows, ARRAY_LEN(render_rows));
}

static void draw_gameplay_page() {
    draw_stat_table(gameplay_rows, ARRAY_LEN(gameplay_rows));
}

static void draw_player_camera_page() {
    draw_stat_table(player_camera_rows, ARRAY_LEN(player_camera_rows));
}

static void draw_misc_page() {
    draw_stat_table(misc_rows, ARRAY_LEN(misc_rows));
}

static void draw_memory_page() {
    draw_stat_table(memory_rows, ARRAY_LEN(memory_rows));
}

void profiler_update(ProfilerUpdateData data) {
    snapshot.frame_ms = delta * 1000.f;
    snapshot.cpu_ms = data.processingTime + C3D_GetProcessingTime();
    snapshot.gpu_ms = C3D_GetDrawingTime();

    // Physics
    snapshot.steps = data.steps;
    
    // Touch
    snapshot.input_x = data.touchPos.px;
    snapshot.input_y = data.touchPos.py;

    // Player
    snapshot.tick = state.player.frame;
    snapshot.ply_pos_x = state.player.x;
    snapshot.ply_pos_y = state.player.y;
    snapshot.ply_vel_x = state.player.vel_x * STEPS_DT;
    snapshot.ply_vel_y = state.player.vel_y * STEPS_DT;

    // Camera
    snapshot.cam_pos_x = state.camera_x;
    snapshot.cam_pos_y = state.camera_y;
    snapshot.cam_itd_y = state.camera_intended_y;
    
    // Heap
    struct mallinfo mi = mallinfo();
    snapshot.linear_free = linearSpaceFree();
    snapshot.heap_used = mi.uordblks;
    snapshot.heap_free = envGetHeapSize() - mi.uordblks;
    snapshot.heap_total = envGetHeapSize();

    profiler_handle_input(data.kDown);
}

#define BG_COL (ABGR8(0, 0, 0, 127))

void profiler_draw(void) {
    C2D_DrawRectangle(0, 0, 0, SCREEN_BOT_WIDTH, SCREEN_HEIGHT, BG_COL, BG_COL, BG_COL, BG_COL);
    draw_page_header();

    switch (current_page) {
        case PROFILER_PAGE_OVERVIEW:
            draw_overview_page();
            break;
        case PROFILER_PAGE_RENDER:
            draw_render_page();
            break;
        case PROFILER_PAGE_GAMEPLAY:
            draw_gameplay_page();
            break;
        case PROFILER_PAGE_PLAYER_CAMERA:
            draw_player_camera_page();
            break;
        case PROFILER_PAGE_MISC:
            draw_misc_page();
            break;
        case PROFILER_PAGE_MEMORY:
            draw_memory_page();
            break;
        default:
            break;
    }

    // Reset
    snapshot = (ProfilerSnapshot) { 0 };
}

#undef BG_COL