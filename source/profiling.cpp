#include "profiling.hpp"
#include "3ds/services/hid.h"
#include "arena.hpp"
#include "fonts/chatFont.hpp"
#include "main.hpp"
#include "math_helpers.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/settings_hub/settings.hpp"
#include "state.hpp"

ProfilerSnapshot snapshot;
static ProfilerSnapshot internalSnapshot;

typedef enum {
    PROFILER_PAGE_OVERVIEW,
    PROFILER_PAGE_RENDER,
    PROFILER_PAGE_GAMEPLAY,
    PROFILER_PAGE_PLAYER_CAMERA,
    PROFILER_PAGE_MISC,
    PROFILER_PAGE_MEMORY,
    PROFILER_PAGE_COUNT
} ProfilerPage;

static int currentPage = (int)PROFILER_PAGE_OVERVIEW;

#define PROFILER_SCALE 0.8f
#define PROFILER_ROW_HEIGHT 12.f
#define PROFILER_LABEL_X 0.f
#define PROFILER_VALUE_X 180.f
#define PROFILER_SEC_VALUE_X 260.f
#define PROFILER_START_Y 25.f

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
    PROFILER_STAT_PLAYER_STEP_TRIGGER,
    PROFILER_STAT_PLAYER_STEPS,
    PROFILER_STAT_PLAYER_COLLISIONS,
    PROFILER_STAT_PLAYER_COLLISION_CHECKS,

    PROFILER_STAT_RENDER_CREATING,
    PROFILER_STAT_RENDER_SORTING,
    PROFILER_STAT_RENDER_TINT,
    PROFILER_STAT_RENDER_DRAWING,

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
    PROFILER_STAT_LEVEL_ARENA_USED,
    PROFILER_STAT_LEVEL_ARENA_FREE,
    PROFILER_STAT_LEVEL_ARENA_TOTAL,
} ProfilerStat;

typedef struct {
    const char *label;
    ProfilerStat stat;
    ProfilerValueType valueType;
} ProfilerRow;

static const ProfilerRow overviewRows[] = {
    { "FPS",       PROFILER_STAT_FPS,       PROFILER_VALUE_FPS },
    { "CPU",       PROFILER_STAT_CPU,       PROFILER_VALUE_MS_PERCENTAGE },
    { "GPU",       PROFILER_STAT_GPU,       PROFILER_VALUE_MS_PERCENTAGE },
    {},
    { "Rendering", PROFILER_STAT_RENDERING, PROFILER_VALUE_MS_PERCENTAGE },
    { "Physics",   PROFILER_STAT_PHYSICS,   PROFILER_VALUE_MS_PERCENTAGE },
    { "Triggers",  PROFILER_STAT_TRIGGERS,  PROFILER_VALUE_MS_PERCENTAGE },
};

static const ProfilerRow renderRows[] = {
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
    { "- Total",    PROFILER_STAT_OPTIM_COUNT,    PROFILER_VALUE_INTEGER},
};

static const ProfilerRow gameplayRows[] = {
    { "Collisions", PROFILER_STAT_PLAYER_COLLISIONS,       PROFILER_VALUE_INTEGER },
    { "Checks",     PROFILER_STAT_PLAYER_COLLISION_CHECKS, PROFILER_VALUE_INTEGER },
    { "Steps",      PROFILER_STAT_PLAYER_STEPS,            PROFILER_VALUE_INTEGER },
    {},
    { "Physics",        PROFILER_STAT_PHYSICS,            PROFILER_VALUE_MS_PERCENTAGE },
    { "- Player",       PROFILER_STAT_PLAYER,             PROFILER_VALUE_MS_PERCENTAGE },
    { "- Collision",    PROFILER_STAT_PLAYER_COLLISION,   PROFILER_VALUE_MS_PERCENTAGE },
    { "- Handler",      PROFILER_STAT_PLAYER_HANDLER,     PROFILER_VALUE_MS_PERCENTAGE },
    { "- Step triggers",PROFILER_STAT_PLAYER_STEP_TRIGGER,PROFILER_VALUE_MS_PERCENTAGE },
    {},
    { "Triggers",  PROFILER_STAT_TRIGGERS,  PROFILER_VALUE_MS_PERCENTAGE },
};

static const ProfilerRow playerCameraRows[] = {
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

static const ProfilerRow miscRows[] = {
    { "Input" },
    { "- X",   PROFILER_STAT_INPUT_X, PROFILER_VALUE_INTEGER },
    { "- Y",   PROFILER_STAT_INPUT_Y, PROFILER_VALUE_INTEGER }
};

static const ProfilerRow memoryRows[] = {
    { "Linear free", PROFILER_STAT_LINEAR_FREE, PROFILER_VALUE_HEX },
    {},
    { "Heap used",   PROFILER_STAT_HEAP_USED,   PROFILER_VALUE_HEX },
    { "Heap free",   PROFILER_STAT_HEAP_FREE,   PROFILER_VALUE_HEX },
    { "Heap total",  PROFILER_STAT_HEAP_TOTAL,  PROFILER_VALUE_HEX },
    {},
    { "Level arena used",   PROFILER_STAT_LEVEL_ARENA_USED,   PROFILER_VALUE_HEX },
    { "Level arena free",   PROFILER_STAT_LEVEL_ARENA_FREE,   PROFILER_VALUE_HEX },
    { "Level arena total",  PROFILER_STAT_LEVEL_ARENA_TOTAL,  PROFILER_VALUE_HEX }
};

static const char *pageNames[PROFILER_PAGE_COUNT] = {
    "Overview",
    "Render",
    "Gameplay",
    "Player/Camera",
    "Miscellaneous",
    "Memory",
};

static void profilerHandleInput(u32 kDown) {
    if (kDown & KEY_LEFT) {
        if (currentPage == PROFILER_PAGE_OVERVIEW) currentPage = PROFILER_PAGE_COUNT - 1;
        else currentPage--;
    }

    if (kDown & KEY_RIGHT) {
        currentPage = (currentPage + 1) % PROFILER_PAGE_COUNT;
    }
}

static void drawPageHeader(void) {
    drawText(
        &chatFontFontCharset,
        &chatFontSheet,
        0, 10,
        PROFILER_SCALE, PROFILER_SCALE,
        0,
        true,
        "[%d/%d] %s",
        currentPage + 1,
        PROFILER_PAGE_COUNT,
        pageNames[currentPage]
    );
}

static float profilerStatValue(ProfilerStat stat) {
    switch (stat) {
        case PROFILER_STAT_FRAME: return internalSnapshot.frameMs;
        case PROFILER_STAT_FPS: return internalSnapshot.frameMs > 0 ? 1000.f / internalSnapshot.frameMs : 0.f;
        case PROFILER_STAT_CPU: return internalSnapshot.cpuMs;
        case PROFILER_STAT_GPU: return internalSnapshot.gpuMs;

        case PROFILER_STAT_PHYSICS: return internalSnapshot.physicsMs;
        case PROFILER_STAT_TRIGGERS: return internalSnapshot.triggersMs;
        case PROFILER_STAT_COLLISION: return internalSnapshot.collisionMs;
        case PROFILER_STAT_PARTICLES: return internalSnapshot.particlesMs;
        case PROFILER_STAT_RENDERING: return internalSnapshot.renderingMs;

        case PROFILER_STAT_PLAYER: return internalSnapshot.playMs;
        case PROFILER_STAT_PLAYER_COLLISION: return internalSnapshot.collisionMs;
        case PROFILER_STAT_PLAYER_HANDLER: return internalSnapshot.handlerMs;
        case PROFILER_STAT_PLAYER_STEP_TRIGGER: return internalSnapshot.stepTriggerMs;

        case PROFILER_STAT_RENDER_CREATING: return internalSnapshot.creatingMs;
        case PROFILER_STAT_RENDER_SORTING: return internalSnapshot.sortingMs;
        case PROFILER_STAT_RENDER_TINT: return internalSnapshot.tintMs;
        case PROFILER_STAT_RENDER_DRAWING: return internalSnapshot.drawingMs;

        case PROFILER_STAT_PLAYER_X: return internalSnapshot.plyPosX;
        case PROFILER_STAT_PLAYER_Y: return internalSnapshot.plyPosY;
        case PROFILER_STAT_PLAYER_VX: return internalSnapshot.plyVelX;
        case PROFILER_STAT_PLAYER_VY: return internalSnapshot.plyVelY;
        
        case PROFILER_STAT_CAMERA_X: return internalSnapshot.camPosX;
        case PROFILER_STAT_CAMERA_Y: return internalSnapshot.camPosY;
        case PROFILER_STAT_CAMERA_INTENDED_Y: return internalSnapshot.camItdY;

        default: return 0.f;
    }
}

static unsigned int profilerStatInteger(ProfilerStat stat) {
    switch (stat) {
        case PROFILER_STAT_PLAYER_STEPS: return internalSnapshot.steps;
        case PROFILER_STAT_PLAYER_COLLISIONS: return internalSnapshot.collisions;
        case PROFILER_STAT_PLAYER_COLLISION_CHECKS: return internalSnapshot.collisionChecks;

        case PROFILER_STAT_OPTIM_COUNT: return internalSnapshot.drawCount;

        case PROFILER_STAT_INPUT_X: return internalSnapshot.inputX;
        case PROFILER_STAT_INPUT_Y: return internalSnapshot.inputY;

        case PROFILER_STAT_PLAYER_TICK: return internalSnapshot.tick;

        case PROFILER_STAT_LINEAR_FREE: return internalSnapshot.linearFree;
        case PROFILER_STAT_HEAP_USED: return internalSnapshot.heapUsed;
        case PROFILER_STAT_HEAP_FREE: return internalSnapshot.heapFree;
        case PROFILER_STAT_HEAP_TOTAL: return internalSnapshot.heapTotal;

        case PROFILER_STAT_LEVEL_ARENA_USED: return internalSnapshot.levelArenaUsed;
        case PROFILER_STAT_LEVEL_ARENA_FREE: return internalSnapshot.levelArenaFree;
        case PROFILER_STAT_LEVEL_ARENA_TOTAL: return internalSnapshot.levelArenaTotal;
        default: return 0;
    }
}

static void drawStatValue(const ProfilerRow *row, float y) {
    switch (row->valueType) {
        case PROFILER_VALUE_FLOAT:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.2f",
                profilerStatValue(row->stat));
            break;
        case PROFILER_VALUE_FPS:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.1f FPS",
                profilerStatValue(row->stat));
            break;
        case PROFILER_VALUE_MS_PERCENTAGE:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.2f%%",
                profilerStatValue(row->stat) * 6.f);
            break;
        case PROFILER_VALUE_INTEGER:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%u",
                profilerStatInteger(row->stat));
            break;
        case PROFILER_VALUE_HEX:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "0x%X",
                profilerStatInteger(row->stat));
            break;
        default: 
            break;
    }
}

static void drawSecondaryStatValue(const ProfilerRow *row, float y) {
    switch (row->valueType) {
        case PROFILER_VALUE_MS_PERCENTAGE:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_SEC_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "(%.2f ms)",
                profilerStatValue(row->stat));
            break;
        default:
            break;
    }
}

static void drawStatTable(const ProfilerRow *rows, unsigned int rowCount) {
    for (unsigned int rowIndex = 0; rowIndex < rowCount; rowIndex++) {
        float y = PROFILER_START_Y + rowIndex * PROFILER_ROW_HEIGHT;

        // Skip empty
        if (rows[rowIndex].label == NULL) {
            continue;
        }

        drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_LABEL_X, y,
            PROFILER_SCALE, PROFILER_SCALE, 0, true, "%s", rows[rowIndex].label);
        drawStatValue(&rows[rowIndex], y);
        drawSecondaryStatValue(&rows[rowIndex], y);
    }
}

static void drawOverviewPage() {
    drawStatTable(overviewRows, ARRAY_LEN(overviewRows));
}

static void drawRenderPage() {
    drawStatTable(renderRows, ARRAY_LEN(renderRows));
}

static void drawGameplayPage() {
    drawStatTable(gameplayRows, ARRAY_LEN(gameplayRows));
}

static void drawPlayerCameraPage() {
    drawStatTable(playerCameraRows, ARRAY_LEN(playerCameraRows));
}

static void drawMiscPage() {
    drawStatTable(miscRows, ARRAY_LEN(miscRows));
}

static void drawMemoryPage() {
    drawStatTable(memoryRows, ARRAY_LEN(memoryRows));
}

void profilerUpdate(ProfilerUpdateData data) {
    snapshot.frameMs = settingsState.vsync ? delta * 1000.f : MAX(delta * 1000.f, C3D_GetDrawingTime());
    snapshot.cpuMs = data.processingTime + C3D_GetProcessingTime();
    snapshot.gpuMs = C3D_GetDrawingTime();

    // Physics
    snapshot.steps = data.steps;
    
    // Touch
    snapshot.inputX = data.touchPos.px;
    snapshot.inputY = data.touchPos.py;

    // Player
    snapshot.tick = state.player.frame;
    snapshot.plyPosX = state.player.x;
    snapshot.plyPosY = state.player.y;
    snapshot.plyVelX = state.player.velX * STEPS_DT;
    snapshot.plyVelY = state.player.velY * STEPS_DT;

    // Camera
    snapshot.camPosX = state.cameraX;
    snapshot.camPosY = state.cameraY;
    snapshot.camItdY = state.cameraIntendedY;
    
    // Heap
    struct mallinfo mi = mallinfo();
    snapshot.linearFree = linearSpaceFree();
    snapshot.heapUsed = mi.uordblks;
    snapshot.heapFree = envGetHeapSize() - mi.uordblks;
    snapshot.heapTotal = envGetHeapSize();

    snapshot.levelArenaUsed = arenaUsed(&levelArena);
    snapshot.levelArenaFree = arenaAvailable(&levelArena);
    snapshot.levelArenaTotal = arenaTotal(&levelArena);

    if (!gamePaused) internalSnapshot = snapshot;

    profilerHandleInput(data.kDown);
}

#define BG_COL (ABGR8(0, 0, 0, 127))

void profilerDraw(void) {
    C2D_DrawRectangle(0, 0, 0, SCREEN_BOT_WIDTH, SCREEN_HEIGHT, BG_COL, BG_COL, BG_COL, BG_COL);
    drawPageHeader();

    switch (currentPage) {
        case PROFILER_PAGE_OVERVIEW:
            drawOverviewPage();
            break;
        case PROFILER_PAGE_RENDER:
            drawRenderPage();
            break;
        case PROFILER_PAGE_GAMEPLAY:
            drawGameplayPage();
            break;
        case PROFILER_PAGE_PLAYER_CAMERA:
            drawPlayerCameraPage();
            break;
        case PROFILER_PAGE_MISC:
            drawMiscPage();
            break;
        case PROFILER_PAGE_MEMORY:
            drawMemoryPage();
            break;
        default:
            break;
    }

    // Reset
    snapshot = (ProfilerSnapshot) { 0 };
}

#undef BG_COL