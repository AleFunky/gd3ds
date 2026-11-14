#include "profiling.hpp"
#include "3ds/services/hid.h"
#include "arena.hpp"
#include "fonts/chatFont.hpp"
#include "main.hpp"
#include "math_helpers.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/settings_hub/settings.hpp"
#include "state.hpp"

namespace gd3ds
{

ProfilerSnapshot snapshot;
static ProfilerSnapshot internalSnapshot;

enum class ProfilerPage {
    ProfilerPageOverview,
    ProfilerPageRender,
    ProfilerPageGameplay,
    ProfilerPagePlayerCamera,
    ProfilerPageMisc,
    ProfilerPageMemory,
    ProfilerPageCount
};

static int currentPage = (int)static_cast<int>(ProfilerPage::ProfilerPageOverview);

#define PROFILER_SCALE 0.8f
#define PROFILER_ROW_HEIGHT 12.f
#define PROFILER_LABEL_X 0.f
#define PROFILER_VALUE_X 180.f
#define PROFILER_SEC_VALUE_X 260.f
#define PROFILER_START_Y 25.f

enum class ProfilerValueType {
    ProfilerValueNone,
    ProfilerValueFloat,
    ProfilerValueFps,
    ProfilerValueMsPercentage,
    ProfilerValueInteger,
    ProfilerValueHex
};

enum class ProfilerStat {
    ProfilerStatNone,

    ProfilerStatFrame,
    ProfilerStatFps,
    ProfilerStatCpu,
    ProfilerStatGpu,

    ProfilerStatPhysics,
    ProfilerStatTriggers,
    ProfilerStatCollision,
    ProfilerStatParticles,
    ProfilerStatRendering,
    
    ProfilerStatPlayer,
    ProfilerStatPlayerCollision,
    ProfilerStatPlayerHandler,
    ProfilerStatPlayerStepTrigger,
    ProfilerStatPlayerSteps,
    ProfilerStatPlayerCollisions,
    ProfilerStatPlayerCollisionChecks,

    ProfilerStatRenderCreating,
    ProfilerStatRenderSorting,
    ProfilerStatRenderTint,
    ProfilerStatRenderDrawing,

    ProfilerStatOptimCount,
    ProfilerStatOptimInCache,

    ProfilerStatInputX,
    ProfilerStatInputY,

    ProfilerStatPlayerTick,
    ProfilerStatPlayerX,
    ProfilerStatPlayerY,
    ProfilerStatPlayerVx,
    ProfilerStatPlayerVy,

    ProfilerStatCameraX,
    ProfilerStatCameraY,
    ProfilerStatCameraIntendedY,

    ProfilerStatLinearFree,
    ProfilerStatHeapUsed,
    ProfilerStatHeapFree,
    ProfilerStatHeapTotal,
    ProfilerStatLevelArenaUsed,
    ProfilerStatLevelArenaFree,
    ProfilerStatLevelArenaTotal,
};

struct ProfilerRow {
    const char *label;
    ProfilerStat stat;
    ProfilerValueType valueType;
};

static const ProfilerRow overviewRows[] = {
    { "FPS",       ProfilerStat::ProfilerStatFps,       ProfilerValueType::ProfilerValueFps },
    { "CPU",       ProfilerStat::ProfilerStatCpu,       ProfilerValueType::ProfilerValueMsPercentage },
    { "GPU",       ProfilerStat::ProfilerStatGpu,       ProfilerValueType::ProfilerValueMsPercentage },
    {},
    { "Rendering", ProfilerStat::ProfilerStatRendering, ProfilerValueType::ProfilerValueMsPercentage },
    { "Physics",   ProfilerStat::ProfilerStatPhysics,   ProfilerValueType::ProfilerValueMsPercentage },
    { "Triggers",  ProfilerStat::ProfilerStatTriggers,  ProfilerValueType::ProfilerValueMsPercentage },
};

static const ProfilerRow renderRows[] = {
    { "CPU",        ProfilerStat::ProfilerStatCpu,             ProfilerValueType::ProfilerValueMsPercentage },
    { "GPU",        ProfilerStat::ProfilerStatGpu,             ProfilerValueType::ProfilerValueMsPercentage },
    {},
    { "Rendering",  ProfilerStat::ProfilerStatRendering,       ProfilerValueType::ProfilerValueMsPercentage },
    { "- Creating", ProfilerStat::ProfilerStatRenderCreating, ProfilerValueType::ProfilerValueMsPercentage },
    { "- Sorting",  ProfilerStat::ProfilerStatRenderSorting,  ProfilerValueType::ProfilerValueMsPercentage },
    { "- Tinting",  ProfilerStat::ProfilerStatRenderTint,     ProfilerValueType::ProfilerValueMsPercentage },
    { "- Drawing",  ProfilerStat::ProfilerStatRenderDrawing,  ProfilerValueType::ProfilerValueMsPercentage },
    {},
    { "Particles",  ProfilerStat::ProfilerStatParticles,       ProfilerValueType::ProfilerValueMsPercentage },
    {},
    { "Optim" },
    { "- Total",    ProfilerStat::ProfilerStatOptimCount,    ProfilerValueType::ProfilerValueInteger},
};

static const ProfilerRow gameplayRows[] = {
    { "Collisions", ProfilerStat::ProfilerStatPlayerCollisions,       ProfilerValueType::ProfilerValueInteger },
    { "Checks",     ProfilerStat::ProfilerStatPlayerCollisionChecks, ProfilerValueType::ProfilerValueInteger },
    { "Steps",      ProfilerStat::ProfilerStatPlayerSteps,            ProfilerValueType::ProfilerValueInteger },
    {},
    { "Physics",        ProfilerStat::ProfilerStatPhysics,            ProfilerValueType::ProfilerValueMsPercentage },
    { "- Player",       ProfilerStat::ProfilerStatPlayer,             ProfilerValueType::ProfilerValueMsPercentage },
    { "- Collision",    ProfilerStat::ProfilerStatPlayerCollision,   ProfilerValueType::ProfilerValueMsPercentage },
    { "- Handler",      ProfilerStat::ProfilerStatPlayerHandler,     ProfilerValueType::ProfilerValueMsPercentage },
    { "- Step triggers",ProfilerStat::ProfilerStatPlayerStepTrigger,ProfilerValueType::ProfilerValueMsPercentage },
    {},
    { "Triggers",  ProfilerStat::ProfilerStatTriggers,  ProfilerValueType::ProfilerValueMsPercentage },
};

static const ProfilerRow playerCameraRows[] = {
    { "Player" },
    { "- X",    ProfilerStat::ProfilerStatPlayerX,  ProfilerValueType::ProfilerValueFloat },
    { "- Y",    ProfilerStat::ProfilerStatPlayerY,  ProfilerValueType::ProfilerValueFloat },
    { "- VX",   ProfilerStat::ProfilerStatPlayerVx, ProfilerValueType::ProfilerValueFloat },
    { "- VY",   ProfilerStat::ProfilerStatPlayerVy, ProfilerValueType::ProfilerValueFloat },
    {},
    { "Camera" },
    { "- X",          ProfilerStat::ProfilerStatCameraX,          ProfilerValueType::ProfilerValueFloat },
    { "- Y",          ProfilerStat::ProfilerStatCameraY,          ProfilerValueType::ProfilerValueFloat },
    { "- Intended Y", ProfilerStat::ProfilerStatCameraIntendedY, ProfilerValueType::ProfilerValueFloat }
};

static const ProfilerRow miscRows[] = {
    { "Input" },
    { "- X",   ProfilerStat::ProfilerStatInputX, ProfilerValueType::ProfilerValueInteger },
    { "- Y",   ProfilerStat::ProfilerStatInputY, ProfilerValueType::ProfilerValueInteger }
};

static const ProfilerRow memoryRows[] = {
    { "Linear free", ProfilerStat::ProfilerStatLinearFree, ProfilerValueType::ProfilerValueHex },
    {},
    { "Heap used",   ProfilerStat::ProfilerStatHeapUsed,   ProfilerValueType::ProfilerValueHex },
    { "Heap free",   ProfilerStat::ProfilerStatHeapFree,   ProfilerValueType::ProfilerValueHex },
    { "Heap total",  ProfilerStat::ProfilerStatHeapTotal,  ProfilerValueType::ProfilerValueHex },
    {},
    { "Level arena used",   ProfilerStat::ProfilerStatLevelArenaUsed,   ProfilerValueType::ProfilerValueHex },
    { "Level arena free",   ProfilerStat::ProfilerStatLevelArenaFree,   ProfilerValueType::ProfilerValueHex },
    { "Level arena total",  ProfilerStat::ProfilerStatLevelArenaTotal,  ProfilerValueType::ProfilerValueHex }
};

static const char *pageNames[static_cast<int>(ProfilerPage::ProfilerPageCount)] = {
    "Overview",
    "Render",
    "Gameplay",
    "Player/Camera",
    "Miscellaneous",
    "Memory",
};

static void profilerHandleInput(u32 kDown) {
    if (kDown & KEY_LEFT) {
        if (currentPage == static_cast<int>(ProfilerPage::ProfilerPageOverview)) currentPage = static_cast<int>(ProfilerPage::ProfilerPageCount) - 1;
        else currentPage--;
    }

    if (kDown & KEY_RIGHT) {
        currentPage = (currentPage + 1) % static_cast<int>(ProfilerPage::ProfilerPageCount);
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
        static_cast<int>(ProfilerPage::ProfilerPageCount),
        pageNames[currentPage]
    );
}

static float profilerStatValue(ProfilerStat stat) {
    switch (stat) {
        case ProfilerStat::ProfilerStatFrame: return internalSnapshot.frameMs;
        case ProfilerStat::ProfilerStatFps: return internalSnapshot.frameMs > 0 ? 1000.f / internalSnapshot.frameMs : 0.f;
        case ProfilerStat::ProfilerStatCpu: return internalSnapshot.cpuMs;
        case ProfilerStat::ProfilerStatGpu: return internalSnapshot.gpuMs;

        case ProfilerStat::ProfilerStatPhysics: return internalSnapshot.physicsMs;
        case ProfilerStat::ProfilerStatTriggers: return internalSnapshot.triggersMs;
        case ProfilerStat::ProfilerStatCollision: return internalSnapshot.collisionMs;
        case ProfilerStat::ProfilerStatParticles: return internalSnapshot.particlesMs;
        case ProfilerStat::ProfilerStatRendering: return internalSnapshot.renderingMs;

        case ProfilerStat::ProfilerStatPlayer: return internalSnapshot.playMs;
        case ProfilerStat::ProfilerStatPlayerCollision: return internalSnapshot.collisionMs;
        case ProfilerStat::ProfilerStatPlayerHandler: return internalSnapshot.handlerMs;
        case ProfilerStat::ProfilerStatPlayerStepTrigger: return internalSnapshot.stepTriggerMs;

        case ProfilerStat::ProfilerStatRenderCreating: return internalSnapshot.creatingMs;
        case ProfilerStat::ProfilerStatRenderSorting: return internalSnapshot.sortingMs;
        case ProfilerStat::ProfilerStatRenderTint: return internalSnapshot.tintMs;
        case ProfilerStat::ProfilerStatRenderDrawing: return internalSnapshot.drawingMs;

        case ProfilerStat::ProfilerStatPlayerX: return internalSnapshot.plyPosX;
        case ProfilerStat::ProfilerStatPlayerY: return internalSnapshot.plyPosY;
        case ProfilerStat::ProfilerStatPlayerVx: return internalSnapshot.plyVelX;
        case ProfilerStat::ProfilerStatPlayerVy: return internalSnapshot.plyVelY;
        
        case ProfilerStat::ProfilerStatCameraX: return internalSnapshot.camPosX;
        case ProfilerStat::ProfilerStatCameraY: return internalSnapshot.camPosY;
        case ProfilerStat::ProfilerStatCameraIntendedY: return internalSnapshot.camItdY;

        default: return 0.f;
    }
}

static unsigned int profilerStatInteger(ProfilerStat stat) {
    switch (stat) {
        case ProfilerStat::ProfilerStatPlayerSteps: return internalSnapshot.steps;
        case ProfilerStat::ProfilerStatPlayerCollisions: return internalSnapshot.collisions;
        case ProfilerStat::ProfilerStatPlayerCollisionChecks: return internalSnapshot.collisionChecks;

        case ProfilerStat::ProfilerStatOptimCount: return internalSnapshot.drawCount;

        case ProfilerStat::ProfilerStatInputX: return internalSnapshot.inputX;
        case ProfilerStat::ProfilerStatInputY: return internalSnapshot.inputY;

        case ProfilerStat::ProfilerStatPlayerTick: return internalSnapshot.tick;

        case ProfilerStat::ProfilerStatLinearFree: return internalSnapshot.linearFree;
        case ProfilerStat::ProfilerStatHeapUsed: return internalSnapshot.heapUsed;
        case ProfilerStat::ProfilerStatHeapFree: return internalSnapshot.heapFree;
        case ProfilerStat::ProfilerStatHeapTotal: return internalSnapshot.heapTotal;

        case ProfilerStat::ProfilerStatLevelArenaUsed: return internalSnapshot.levelArenaUsed;
        case ProfilerStat::ProfilerStatLevelArenaFree: return internalSnapshot.levelArenaFree;
        case ProfilerStat::ProfilerStatLevelArenaTotal: return internalSnapshot.levelArenaTotal;
        default: return 0;
    }
}

static void drawStatValue(const ProfilerRow *row, float y) {
    switch (row->valueType) {
        case ProfilerValueType::ProfilerValueFloat:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.2f",
                profilerStatValue(row->stat));
            break;
        case ProfilerValueType::ProfilerValueFps:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.1f FPS",
                profilerStatValue(row->stat));
            break;
        case ProfilerValueType::ProfilerValueMsPercentage:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%.2f%%",
                profilerStatValue(row->stat) * 6.f);
            break;
        case ProfilerValueType::ProfilerValueInteger:
            drawText(&chatFontFontCharset, &chatFontSheet, PROFILER_VALUE_X, y,
                PROFILER_SCALE, PROFILER_SCALE, 1, true, "%u",
                profilerStatInteger(row->stat));
            break;
        case ProfilerValueType::ProfilerValueHex:
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
        case ProfilerValueType::ProfilerValueMsPercentage:
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
        case static_cast<int>(ProfilerPage::ProfilerPageOverview):
            drawOverviewPage();
            break;
        case static_cast<int>(ProfilerPage::ProfilerPageRender):
            drawRenderPage();
            break;
        case static_cast<int>(ProfilerPage::ProfilerPageGameplay):
            drawGameplayPage();
            break;
        case static_cast<int>(ProfilerPage::ProfilerPagePlayerCamera):
            drawPlayerCameraPage();
            break;
        case static_cast<int>(ProfilerPage::ProfilerPageMisc):
            drawMiscPage();
            break;
        case static_cast<int>(ProfilerPage::ProfilerPageMemory):
            drawMemoryPage();
            break;
        default:
            break;
    }

    // Reset
    snapshot = (ProfilerSnapshot) { 0 };
}

#undef BG_COL

}
