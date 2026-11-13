#include "collision.hpp"
#include "icons.hpp"
#include "level_loading.hpp"
#include "player.hpp"
#include "player/trail.hpp"
#include "robot_anim_data.hpp"
#include "spider_anim_data.hpp"
#include <math.h>
#include "practice.hpp"
#include "profiling.hpp"
#include "state.hpp"
#include <citro3d.h>
#include "graphics.hpp"
#include "text.hpp"
#include "fonts/bigFont.hpp"
#include "menus/core/ui_screen.hpp"
#include "math_helpers.hpp"
#include "slope.hpp"
#include "triggers.hpp"
#include "main.hpp"

#include "particles/object_particles.hpp"
#include "particles/circles.hpp"
#include "particles/coin_effect.hpp"
#include "particles/collectable_effect.hpp"
#include "save/saving.hpp"
#include "menus/level_select.hpp"
#include "particles/lightning.hpp"
#include "particles/gravity_sweep.hpp"

Player *player1 = &state.player;
Player *player2 = &state.player2;

const float jumpHeightsTable[SPEED_COUNT][JUMP_TYPES_COUNT][GAMEMODE_COUNT][2] = {
    { // SLOW                        CUBE                              SHIP                                BALL                               UFO            WAVE              ROBOT  },
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {573.48,   458.784},  {0, 0},   {864,      691.2}, {518.4, 414.72} },
    /* YELLOW ORB */ {{573.48,   458.784},  {573.48,   458.784},  {401.435993,  321.148795},  {573.48,   458.784},  {0, 0},   {516.132,  430.9056}, {401.436, 321.149} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-229.392, -183.519}, {-160.574397, -128.463298}, {-229.392, -183.519}, {0, 0},   {-345.6,   -276.48}, {-207.36, -165.888} },
    /* BLUE ORB   */ {{-229.392, -183.519}, {-229.392, -183.519}, {-160.574397, -128.463298}, {-229.392, -183.519}, {0, 0},   {-229.392, -183.519}, {-160.574, -128.463} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0},   {561.6,    449.28}, {362.88, 290.304} },
    /* PINK ORB   */ {{412.884,  330.318},  {212.166,  169.776},  {309.090595,  247.287596},  {240.84,   192.672},  {0, 0},   {412.884,  330.318}, {289.019, 231.223} },
    /* RED ORB    */ {{791.4024, 633.12192},  {573.48, 642.2976},  {537.92423, 430.33939},  {584.9496, 623.94624},  {0, 0},  {660.64896, 551.55917}, {537.924, 430.339}},
    /* RED PAD    */ {{1080,     864},      {544.32,   656.64},   {648,         518.4},       {518.4,    677.376},  {0, 0},   {1080,     864}, {648, 518.4} },
    },
    { // NORMAL
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0},   {864,      691.2}, {518.4, 414.72} },
    /* YELLOW ORB */ {{603.72,   482.976},  {603.72,   482.976},  {422.60399,   338.08319},   {603.72,   482.976},  {0, 0},   {543.348,  434.6784}, {422.604, 338.083} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0},   {-345.6,   -276.48}, {-207.36, -165.888} },
    /* BLUE ORB   */ {{-241.488, -193.185}, {-241.488, -193.18},  {-169.04160,  -135.2295},   {-241.488, -193.185}, {0, 0},   {-241.488, -193.185}, {-169.042, -135.23} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0},   {561.6,    449.28}, {362.88, 290.304} },
    /* PINK ORB   */ {{434.7,    347.76},   {223.398,  178.686},  {325.42019,   260.3286},    {258.984,  207.198},  {0, 0},   {434.7,    347.76}, {304.29, 243.432} },
    /* RED ORB    */ {{833.1336, 666.50688},  {603.72, 676.1664},  {566.28935, 453.03147},  {615.7944, 656.84736},  {0, 0},  {695.48544, 556.38835}, {566.289, 453.031}},
    /* RED PAD    */ {{1080,     864},      {544.32,   656.64},   {648,         518.4},       {518.4,    677.376},  {0, 0},   {1080,     864}, {648, 518.4} },
    },
    { // FAST
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0},   {864,      691.2}, {518.4, 414.72} },
    /* YELLOW ORB */ {{616.68,   493.344},  {616.68,   493.344},  {431.67599,   345.34079},   {616.68,   493.344},  {0, 0},   {555.012,  444.0096}, {431.676, 345.3408} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0},   {-345.6,   -276.48}, {-207.36, -165.888} },
    /* BLUE ORB   */ {{-246.672, -197.343}, {-246.672, -197.343}, {-172.6704,   -138.1401},   {-246.672, -197.343}, {0, 0},   {-246.672, -197.343}, {-172.67, -138.14} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0},   {561.6,    449.28}, {362.88, 290.304} },
    /* PINK ORB   */ {{443.988,  355.212},  {228.15,   182.52},   {332.37539,   265.923},     {258.984,  207.198},  {0, 0},   {443.988,  355.212}, {310.792, 248.648} },
    /* RED ORB    */ {{851.0184, 680.81472},  {616.68, 690.6816},  {578.44583, 462.75666},  {629.0136, 670.94784},  {0, 0},  {710.41536, 568.332288}, {578.446, 462.75667}},
    /* RED PAD    */ {{1080,     864},      {544.32,   656.64},   {648,         518.4},       {518.4,    677.376},  {0, 0},   {1080,     864}, {648, 518.4} },
    },
    { // FASTER
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0},   {864,      691.2}, {518.4, 414.72} },
    /* YELLOW ORB */ {{606.42,   485.136},  {606.42,   485.136},  {424.493993,  339.59519},   {606.42,   485.136},  {0, 0},   {545.778,  436.6224}, {424.494, 339.595} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0},   {-345.6,   -276.48}, {-207.36, -165.888} },
    /* BLUE ORB   */ {{-242.568, -194.049}, {-242.568, -194.049}, {-169.7976,   -135.8343},   {-242.568, -194.049}, {0, 0},   {-242.568, -194.049}, {-169.798, -135.834} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0},   {561.6,    449.28}, {362.88, 290.304} },
    /* PINK ORB   */ {{436.644,  349.272},  {224.37,   179.496},  {326.85659,   261.5004},    {254.718,  203.742},  {0, 0},   {436.644,  349.272}, {305.651, 244.49} },
    /* RED ORB    */ {{836.8596, 669.48768},  {606.42, 679.1904},  {568.82195, 455.05755},  {618.5484, 659.78496},  {0, 0},  {698.59584, 558.87667}, {568.822, 455.058}},
    /* RED PAD    */ {{1080,     864},      {544.32,   656.64},   {648,         518.4},       {518.4,    677.376},  {0, 0},   {1080,     864}, {648, 518.4} },
    },
    { // FASTEST
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0},   {864,      691.2}, {518.4, 414.72} },
    /* YELLOW ORB */ {{606.42,   485.136},  {606.42,   485.136},  {424.493993,  339.59519},   {606.42,   485.136},  {0, 0},   {545.778,  436.6224}, {424.494, 339.595} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0},   {-345.6,   -276.48}, {-207.36, -165.888} },
    /* BLUE ORB   */ {{-242.568, -194.049}, {-242.568, -194.049}, {-169.7976,   -135.8343},   {-242.568, -194.049}, {0, 0},   {-242.568, -194.049}, {-169.798, -135.834} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0},   {561.6,    449.28}, {362.88, 290.304} },
    /* PINK ORB   */ {{436.644,  349.272},  {224.37,   179.496},  {326.85659,   261.5004},    {254.718,  203.742},  {0, 0},   {436.644,  349.272}, {305.651, 244.49} },
    /* RED ORB    */ {{836.8596, 669.48768},  {606.42, 679.1904},  {568.82195, 455.05755},  {618.5484, 659.78496},  {0, 0},  {698.59584, 558.87667}, {568.822, 455.058}},
    /* RED PAD    */ {{1080,     864},      {544.32,   656.64},   {648,         518.4},       {518.4,    677.376},  {0, 0},   {1080,     864}, {648, 518.4} },
    }
};

const float gamemodeHeights[GAMEMODE_COUNT] = {
    999999,  // Cube
    300, // Ship
    240,  // Ball
    300, // Ufo
    300, // Wave
    999999, // Robot
    270 // Spider
};

const int dualGamemodeHeights[GAMEMODE_COUNT] = {
    9,  // Cube
    10, // Ship
    9,  // Ball
    10, // Ufo
    10, // Wave
    9,  // Robot
    9 // Spider
};

const Vec2D slowSpeedSnaps[3] = {
    {
        .x = 120,
        .y = -30
    },
    {
        .x = 90,
        .y = 30
    },
    {
        .x = 60,
        .y = 60
    },
};

const Vec2D normalSpeedSnaps[3] = {
    {
        .x = 150,
        .y = -30
    },
    {
        .x = 120,
        .y = 30
    },
    {
        .x = 90,
        .y = 60
    },
};

const Vec2D fastSpeedSnaps[3] = {
    {
        .x = 180,
        .y = -30
    },
    {
        .x = 150,
        .y = 30
    },
    {
        .x = 120,
        .y = 60
    },
};

const Vec2D fasterSpeedSnaps[3] = {
    {
        .x = 225,
        .y = -30
    },
    {
        .x = 180,
        .y = 30
    },
    {
        .x = 135,
        .y = 60
    },
};

const Vec2D defaultSpeedSnaps[3] = {
    {
        .x = 150,
        .y = -30
    },
    {
        .x = 120,
        .y = 30
    },
    {
        .x = 90,
        .y = 60
    },
};

float snapPlayer(Vec2D diff, Player *player) {
    Vec2D stairs[3];
    float threshold;
    switch (state.speed) {
        case SPEED_SLOW:
            memcpy(&stairs, &slowSpeedSnaps, sizeof(Vec2D) * 3);
            threshold = 1;
            break;
        case SPEED_NORMAL:
            memcpy(&stairs, &normalSpeedSnaps, sizeof(Vec2D) * 3);
            threshold = 1;
            stairs[1].x = (player->mini ? 90 : 120);
            break;
        case SPEED_FAST:
            memcpy(&stairs, &fastSpeedSnaps, sizeof(Vec2D) * 3);
            threshold = 2;
            stairs[1].x = (player->mini ? 90 : 150);
            break;
        case SPEED_FASTER:
            memcpy(&stairs, &fasterSpeedSnaps, sizeof(Vec2D) * 3);
            threshold = 2;
            stairs[1].x = (player->mini ? 90 : 180);
            break;
        case SPEED_FASTEST:
            memcpy(&stairs, &fasterSpeedSnaps, sizeof(Vec2D) * 3);
            threshold = 2;
            stairs[1].x = (player->mini ? 90 : 180);
            break;
        default:
            memcpy(&stairs, &defaultSpeedSnaps, sizeof(Vec2D) * 3);
            threshold = 1;
    }   

    // Check snaps
    for (int i = 0; i < 3; i++) {
        Vec2D stair = stairs[i];
        if (fabsf(diff.x - stair.x) <= threshold && fabsf(diff.y - stair.y) <= threshold) {
            return threshold;
        }
    }
    
    return 0;
}

void trySnap(int block, Player *player) {
    Vec2D diff;
    int snapBlock = player->snapData.objectID;

    if (snapBlock >= 0) {
        diff.x = objects.x[block] - objects.x[snapBlock];
        diff.y = objects.y[block] - objects.y[snapBlock];
        diff.y = grav(player, diff.y);
        float threshold = snapPlayer(diff, player);
        if (threshold > 0) {
            // Snap the player up to threshold
            player->x = clampf(
                objects.x[block] + player->snapData.playerSnapDiff,
                player->x - threshold,
                player->x + threshold
            );

            player->snapData.snappedObj = block;
        }
    }
}

#define Y true
#define N false

// Table that says if gravity changes should affect the other gamemode
static const bool gamemodeLinkedGravity[GAMEMODE_COUNT][GAMEMODE_COUNT] = {
    /* CUBE  */ { Y, N, N, N, Y, N, N},
    /* SHIP  */ { N, Y, N, N, N, N, N},
    /* BALL  */ { N, N, Y, N, N, N, N},
    /* UFO   */ { N, N, N, Y, N, N, N},
    /* WAVE  */ { Y, N, N, N, Y, N, N},
    /* ROBOT */ { N, N, N, N, N, Y, N},
    /* SPIDER */ { N, N, N, N, N, N, Y},
};

#undef Y
#undef N

void flipOtherPlayer(int currentPlayer) {
    if (!levelInfo.twoPlayerMode && state.dual && gamemodeLinkedGravity[state.player.gamemode][state.player2.gamemode] && state.player.upsideDown == state.player2.upsideDown) {
        if (currentPlayer == 0) {
            state.player2.upsideDown = !state.player.upsideDown;
            state.player2.velY /= -2;
            state.player2.ceilingInvTime = CEILING_INVUL_TIME;
        } else {
            state.player.upsideDown = !state.player2.upsideDown;
            state.player.velY /= -2;
            state.player.ceilingInvTime = CEILING_INVUL_TIME;
        }
    }
}

void doBallReflection() {
    Player *player1 = &state.player;
    Player *player2 = &state.player2;
    if (state.dual && player1->gamemode == GAMEMODE_BALL && player2->gamemode == GAMEMODE_BALL) {
        if (player1->upsideDown == player2->upsideDown) {
            bool ballsIntersecting = intersect(
                player1->x, player1->y, player1->width, player1->height, 0,
                player2->x, player2->y, player2->width, player2->height, 0
            );

            if (ballsIntersecting) {
                int currentPlayer = state.currentPlayer;
                if (player1->onGround || player1->onCeiling) {
                    state.currentPlayer = 0;

                    UseEffect *effect = addUseEffect(player1->x, player1->y, USE_EFFECT_OBJ_P1, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        Color p1White = getWhiteIfBlack(p1Color);
                        effect->def.colorR = p1White.r / 255.f;
                        effect->def.colorG = p1White.g / 255.f;
                        effect->def.colorB = p1White.b / 255.f;
                    }

                    player2->velY = jumpHeightsTable[state.speed][JUMP_BLUE_PAD][player2->gamemode][player2->mini];
                    player2->upsideDown ^= 1;
                } else if (player2->onGround || player2->onCeiling) {
                    state.currentPlayer = 1;

                    UseEffect *effect = addUseEffect(player2->x, player2->y, USE_EFFECT_OBJ_P2, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        Color p2White = getWhiteIfBlack(p2Color);
                        effect->def.colorR = p2White.r / 255.f;
                        effect->def.colorG = p2White.g / 255.f;
                        effect->def.colorB = p2White.b / 255.f;
                    }


                    player1->velY = jumpHeightsTable[state.speed][JUMP_BLUE_PAD][player1->gamemode][player1->mini];
                    player1->upsideDown ^= 1;
                }
                state.currentPlayer = currentPlayer;
            }
        }   
    }
}

void setDualBounds() {
    int height = MAX(dualGamemodeHeights[state.player.gamemode],
                 dualGamemodeHeights[state.player2.gamemode]);

    float inBlockY = fmodf(state.dualPortalY, 30);
    int groundOffset = (ceilf(((float) height + 1) / 2) - 1) * 30;
    state.groundY = fmaxf(0, floorf((state.dualPortalY - groundOffset) / 30.f)) * 30;


    // Shift down if odd height and in the top half
    if (height % 2 != 0) {
        if (inBlockY < 15) {
            state.groundY = fmaxf(0, state.groundY - 30);
        }
    }

    state.ceilingY = state.groundY + (height * 30.f);
    setIntendedCeiling();
}

void setupDual() {
    memcpy(&state.player2, &state.player, sizeof(Player));
    memset(&state.p1TrailData[1], 0, sizeof(P1Trail) * P1_TRAIL_DURATION);
    state.p1TrailPos[1] = 0;
    state.player2.upsideDown = state.player.upsideDown ^ 1;
    UseEffect *effect = addUseEffect(player1->x, player1->y, USE_EFFECT_OBJ_P1, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
    if (effect) {
        effect->def.colorR = getWhiteIfBlack(p1Color).r;
        effect->def.colorG = getWhiteIfBlack(p1Color).g;
        effect->def.colorB = getWhiteIfBlack(p1Color).b;
    }
    UseEffect *effect2 = addUseEffect(player2->x, player2->y, USE_EFFECT_OBJ_P2, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
    if (effect2) {
        effect2->def.colorR = getWhiteIfBlack(p2Color).r;
        effect2->def.colorG = getWhiteIfBlack(p2Color).g;
        effect2->def.colorB = getWhiteIfBlack(p2Color).b;
    }
}

static float snapRotation360(float rotation) {
    if (rotation <= 180.f) {
        if (rotation < -180.f) rotation += 360.f;
    } else {
        rotation -= 360.f;
    }
    return rotation;
}

static float dashOrbAngle(int obj) {
    float rotation = adjustAngleY(objects.rotation[obj], objects.flippedH[obj]);

    float angle = snapRotation360(fmodf(-rotation, 360.f));

    if (fabsf(angle) > 90.f) angle = snapRotation360((angle <= 0.f ? -180.f : 180.f) - angle);

    if (angle > 70.f) angle = (angle >= 180.f) ? -70.f : 70.f;
    else if (angle < -70.f) angle = (angle > -180.f) ? -70.f : 70.f;

    return angle;
}

static void collectPickupItem(int obj) {
    PickupItem *pickup = getPickupItem(obj);
    if (!pickup || !pickup->isPickup || (objects.flags[obj] & FLAG_TOGGLED)) return;
    addItemCount(pickup->itemID, (pickup->subCount ? -1 : 1));
}

static void spawnCollectableFx(int obj) {
    const GameObject *gameObj = &gameObjects[objects.id[obj]];
    int detailTex = gameObj->childCount > 0 ? gameObj->children[0].texture : -1;
    float detailX = gameObj->childCount > 0 ? gameObj->children[0].x : 0.0f;
    float detailY = gameObj->childCount > 0 ? gameObj->children[0].y : 0.0f;
    if (objects.id[obj] == KEY_OBJ) detailY = 4.0f / 3.0f;

    collectableEffectSpawn(objects.x[obj], objects.y[obj],
                             gameObj->texture, detailTex,
                             objects.colChannel[obj],
                             objects.detailColChannel[obj],
                             detailX, detailY,
                             objects.scaleX[obj], objects.scaleY[obj],
                             objects.flippedH[obj], objects.flippedV[obj]);

    ParticleDefinition prevCfg = brickDestroyParticles.cfg;
    brickDestroyParticles.cfg.startColorRed = objects.mainColor[obj].r / 255.f;
    brickDestroyParticles.cfg.startColorGreen = objects.mainColor[obj].g / 255.f;
    brickDestroyParticles.cfg.startColorBlue = objects.mainColor[obj].b / 255.f;
    brickDestroyParticles.cfg.startColorAlpha = 127.0f / 255.0f;
    brickDestroyParticles.emitterX = objects.x[obj];
    brickDestroyParticles.emitterY = objects.y[obj];
    spawnMultipleParticles(&brickDestroyParticles, 10);
    brickDestroyParticles.cfg = prevCfg;
}

// Handle collision with special objects (orbs, pads, portals)
void handleSpecialHitbox(Player *player, int obj, const ObjectHitbox *hitbox) {
    switch (objects.id[obj]) {
        case HEART_COLLECTABLE:
        case POTION_COLLECTABLE:
        case SKULL_COLLECTABLE:
        case KEY_OBJ:
            if (!GET_ACTIVATED(obj)) {
                collectPickupItem(obj);
                SET_ACTIVATED(obj, true);
                spawnCollectableFx(obj);
                objects.flags[obj] |= FLAG_TOGGLED;
            }
            break;
        case SMALL_COIN:
            if (!GET_ACTIVATED(obj)) {
                collectPickupItem(obj);
                SET_ACTIVATED(obj, true);
                objects.flags[obj] |= FLAG_TOGGLED;
            }
            break;
        case YELLOW_PAD: {
            if (!GET_ACTIVATED(obj) || ((objects.flags[obj] & FLAG_MULTI_ACTIVATE) && !GET_COLLIDED(obj))) {
                if (state.showPlayer) motionTrailResumeStroke(trail);
                player->velY = jumpHeightsTable[state.speed][JUMP_YELLOW_PAD][player->gamemode][player->mini];
                player->onGround = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->jumped = true;
                if (player->gamemode == GAMEMODE_ROBOT) setAnimation(player, ROBOT_ANIM_JUMP_START);
                if (player->gamemode == GAMEMODE_SPIDER) setAnimation(player, SPIDER_ANIM_FALL);
                SET_ACTIVATED(obj, true);
                updateRotationDirection(player);
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &padUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 200 / 255.f;
                    effect->def.colorB = 0;
                }
            break;
            }
        }
        case PINK_PAD: {
            if (!GET_ACTIVATED(obj) || ((objects.flags[obj] & FLAG_MULTI_ACTIVATE) && !GET_COLLIDED(obj))) {
                if (state.showPlayer) motionTrailResumeStroke(trail);
                player->velY = jumpHeightsTable[state.speed][JUMP_PINK_PAD][player->gamemode][player->mini];
                player->onGround = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->jumped = true;
                if (player->gamemode == GAMEMODE_ROBOT) setAnimation(player, ROBOT_ANIM_JUMP_START);
                if (player->gamemode == GAMEMODE_SPIDER) setAnimation(player, SPIDER_ANIM_FALL);
                SET_ACTIVATED(obj, true);
                updateRotationDirection(player);
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &padUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 50 / 255.f;
                    effect->def.colorB = 255 / 255.f;
                }
                break;
            }
            break;
        }
        case RED_PAD: {
            if (!GET_ACTIVATED(obj) || ((objects.flags[obj] & FLAG_MULTI_ACTIVATE) && !GET_COLLIDED(obj))) {
                if (state.showPlayer) motionTrailResumeStroke(trail);
                player->velY = jumpHeightsTable[state.speed][JUMP_RED_PAD][player->gamemode][player->mini];
                player->onGround = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->jumped = true;
                player->velocityUncapped = true;
                if (player->gamemode == GAMEMODE_ROBOT) setAnimation(player, ROBOT_ANIM_JUMP_START);
                if (player->gamemode == GAMEMODE_SPIDER) setAnimation(player, SPIDER_ANIM_FALL);
                SET_ACTIVATED(obj, true);
                updateRotationDirection(player);
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &padUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 200 / 255.f;
                    effect->def.colorB = 0;
                }
            }
            break;
        }
        case BLUE_PAD: {
            if (GET_ACTIVATED(obj)) player->gravObjID = obj;
            if (!GET_ACTIVATED(obj) || ((objects.flags[obj] & FLAG_MULTI_ACTIVATE) && !GET_COLLIDED(obj))) {
                float rotation = adjustAngle(objects.rotation[obj], objects.flippedV[obj], objects.flippedH[obj]);
                if ((rotation < 90 || rotation > 270) && player->upsideDown)
                    break;
                    
                if ((rotation > 90 && rotation < 270) && !player->upsideDown)
                    break;

                // No double flip in the same frame
                if (state.oldPlayer.upsideDown != player->upsideDown) 
                    break;

                if (state.showPlayer) motionTrailResumeStroke(trail);
                if (player->gamemode == GAMEMODE_WAVE) motionTrailAddWavePoint(waveTrail);
                player->leftGround = true;

                player->gravObjID = obj;
                updateRotationDirection(player);

                player->velY = jumpHeightsTable[state.speed][JUMP_BLUE_PAD][player->gamemode][player->mini];
                player->upsideDown ^= 1;

                startGravitySweep(!player->upsideDown);

                flipOtherPlayer(state.currentPlayer);
                player->onGround = false;
                player->inverseRotation = false;
                player->ceilingInvTime = CEILING_INVUL_TIME;
                player->jumped = true;
                player->robotAirTime = 999.f;

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &padUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 0 / 255.f;
                    effect->def.colorG = 255 / 255.f;
                    effect->def.colorB = 255 / 255.f;
                }

                SET_ACTIVATED(obj, true);
                break;
            }
            break;
        }
        case YELLOW_ORB: {
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if ((!GET_ACTIVATED(obj) || (objects.flags[obj] & FLAG_MULTI_ACTIVATE)) && (currInput.holdJump) && player->bufferingState == BUFFER_READY) {
                if (state.showPlayer) motionTrailResumeStroke(trail);
                player->velY = jumpHeightsTable[state.speed][JUMP_YELLOW_ORB][player->gamemode][player->mini];
                
                player->ballRotationSpeed = -BALL_SLOW_ROTATION;
                
                player->onGround = false;
                player->onCeiling = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->bufferingState = BUFFER_END;
                player->jumped = true;
                updateRotationDirection(player);
                if (player->gamemode == GAMEMODE_ROBOT) setAnimation(player, ROBOT_ANIM_JUMP_START);
                if (player->gamemode == GAMEMODE_SPIDER) setAnimation(player, SPIDER_ANIM_JUMP);
                
                state.currentData.jumps++;
                
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 200 / 255.f;
                    effect->def.colorB = 0;
                }
                
                player->lastOrbPressed = obj;

                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case PINK_ORB: {
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if ((!GET_ACTIVATED(obj) || (objects.flags[obj] & FLAG_MULTI_ACTIVATE)) && (currInput.holdJump) && player->bufferingState == BUFFER_READY) {
                if (state.showPlayer) motionTrailResumeStroke(trail);
                player->velY = jumpHeightsTable[state.speed][JUMP_PINK_ORB][player->gamemode][player->mini];
                
                player->ballRotationSpeed = -BALL_SLOW_ROTATION;
                
                player->onGround = false;
                player->onCeiling = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->bufferingState = BUFFER_END;
                player->jumped = true;
                updateRotationDirection(player);
                if (player->gamemode == GAMEMODE_ROBOT) setAnimation(player, ROBOT_ANIM_JUMP_START);
                if (player->gamemode == GAMEMODE_SPIDER) setAnimation(player, SPIDER_ANIM_JUMP);

                state.currentData.jumps++;

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 50 / 255.f;
                    effect->def.colorB = 255 / 255.f;
                }

                player->lastOrbPressed = obj;
                
                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case RED_ORB: {
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if ((!GET_ACTIVATED(obj) || (objects.flags[obj] & FLAG_MULTI_ACTIVATE)) && (currInput.holdJump) && player->bufferingState == BUFFER_READY) {
                if (state.showPlayer) motionTrailResumeStroke(trail);
                player->velY = jumpHeightsTable[state.speed][JUMP_RED_ORB][player->gamemode][player->mini];
                
                player->ballRotationSpeed = -BALL_SLOW_ROTATION;
                
                player->onGround = false;
                player->onCeiling = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->bufferingState = BUFFER_END;
                player->jumped = true;
                player->velocityUncapped = true;
                updateRotationDirection(player);
                if (player->gamemode == GAMEMODE_ROBOT) setAnimation(player, ROBOT_ANIM_JUMP_START);
                if (player->gamemode == GAMEMODE_SPIDER) setAnimation(player, SPIDER_ANIM_JUMP);
                
                state.currentData.jumps++;
                
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 100 / 255.f;
                    effect->def.colorB = 0;
                }
                
                player->lastOrbPressed = obj;

                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case BLACK_ORB: {
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if ((!GET_ACTIVATED(obj) || (objects.flags[obj] & FLAG_MULTI_ACTIVATE)) && (currInput.holdJump) && player->bufferingState == BUFFER_READY) {
                if (state.showPlayer) motionTrailResumeStroke(trail);
                float blackVel = (player->gamemode == GAMEMODE_UFO) ? -604.8f : (playerGamemodeIsFlying(player) ? -756.f : -810.f);
                player->velY = blackVel;
                
                player->ballRotationSpeed = -BALL_SLOW_ROTATION;
                
                player->onGround = false;
                player->onCeiling = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->bufferingState = BUFFER_END;
                player->jumped = true;
                player->velocityUncapped = true;
                updateRotationDirection(player);
                if (player->gamemode == GAMEMODE_ROBOT) setAnimation(player, ROBOT_ANIM_JUMP_START);
                
                state.currentData.jumps++;
                
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    Color color = channels[CHANNEL_LBG].color;
                    effect->def.colorR = color.r / 255.f;
                    effect->def.colorG = color.g / 255.f;
                    effect->def.colorB = color.b / 255.f;
                }

                player->lastOrbPressed = obj;

                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case DASH_ORB: {
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if ((!GET_ACTIVATED(obj) || (objects.flags[obj] & FLAG_MULTI_ACTIVATE)) && (currInput.holdJump) && player->bufferingState == BUFFER_READY && !player->dashing) {
                if (player->gamemode != GAMEMODE_WAVE) motionTrailStopStroke(trail);

                player->onGround = false;
                player->onCeiling = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->robotAirTime = 999.f;
                player->bufferingState = BUFFER_END;
                
                player->dashing = true;
                player->dashAngle = dashOrbAngle(obj);
                player->dashSlope = sinf(DegToRad(player->dashAngle)) / fabsf(cosf(DegToRad(player->dashAngle)));
                player->rotation = -player->dashAngle;
                player->velY = 0;

                addDashBoom(objects.x[obj], objects.y[obj], -player->dashAngle);
                playerFlash(player, 0.2f, 0.1f);

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 0 / 255.f;
                    effect->def.colorG = 255 / 255.f;
                    effect->def.colorB = 0;
                }

                player->lastOrbPressed = obj;

                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case GRAVITY_DASH_ORB: {
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if ((!GET_ACTIVATED(obj) || (objects.flags[obj] & FLAG_MULTI_ACTIVATE)) && (currInput.holdJump) && player->bufferingState == BUFFER_READY && !player->dashing) {
                player->ceilingInvTime = CEILING_INVUL_TIME;
                if (player->gamemode != GAMEMODE_BALL && state.showPlayer) motionTrailResumeStroke(trail);
                if (player->gamemode == GAMEMODE_WAVE) motionTrailAddWavePoint(waveTrail);
                player->velY /= -2;
                player->upsideDown = !player->upsideDown;
                updateRotationDirection(player);
                player->robotAirTime = 999.f;
                
                player->onGround = false;
                player->onCeiling = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->bufferingState = BUFFER_END;

                if (player->gamemode == GAMEMODE_UFO)
                    player->rotation = 0;
                else
                    player->snapRotation = true;

                player->leftGround = true;
                flipOtherPlayer(state.currentPlayer);

                player->dashing = true;
                player->dashAngle = dashOrbAngle(obj);
                player->dashSlope = sinf(DegToRad(player->dashAngle)) / fabsf(cosf(DegToRad(player->dashAngle)));
                player->rotation = -player->dashAngle;
                player->velY = 0;

                addGravityDashBoom(objects.x[obj], objects.y[obj], -player->dashAngle);
                playerFlash(player, 0.2f, 0.1f);

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 0 / 255.f;
                    effect->def.colorB = 255 / 255.f;
                }

                player->lastOrbPressed = obj;

                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case BLUE_ORB: {
            if (GET_ACTIVATED(obj)) player->gravObjID = obj;
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if ((!GET_ACTIVATED(obj) || (objects.flags[obj] & FLAG_MULTI_ACTIVATE)) && (currInput.holdJump) && player->bufferingState == BUFFER_READY) {    
                if (state.showPlayer) motionTrailResumeStroke(trail);
                if (player->gamemode == GAMEMODE_WAVE) motionTrailAddWavePoint(waveTrail);
                player->gravObjID = obj;
                updateRotationDirection(player); 
                
                player->velY = jumpHeightsTable[state.speed][JUMP_BLUE_ORB][player->gamemode][player->mini];
                player->upsideDown ^= 1;

                startGravitySweep(!player->upsideDown);

                flipOtherPlayer(state.currentPlayer);
                
                state.currentData.jumps++;
                
                player->ballRotationSpeed = -BALL_SLOW_ROTATION;
                
                player->onGround = false;
                player->onCeiling = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->bufferingState = BUFFER_END;
                player->ceilingInvTime = CEILING_INVUL_TIME;
                player->jumped = true;
                if (player->gamemode == GAMEMODE_SPIDER) setAnimation(player, SPIDER_ANIM_JUMP);
                
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 0 / 255.f;
                    effect->def.colorG = 255 / 255.f;
                    effect->def.colorB = 255 / 255.f;
                }

                player->lastOrbPressed = obj;

                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case GREEN_ORB: {
            if (GET_ACTIVATED(obj)) player->gravObjID = obj;
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if ((!GET_ACTIVATED(obj) || (objects.flags[obj] & FLAG_MULTI_ACTIVATE)) && (currInput.holdJump) && player->bufferingState == BUFFER_READY) {
                if (state.showPlayer) motionTrailResumeStroke(trail);
                player->gravObjID = obj;

                player->upsideDown ^= 1;

                startGravitySweep(!player->upsideDown);

                player->velY = jumpHeightsTable[state.speed][JUMP_YELLOW_ORB][player->gamemode][player->mini];
                
                updateRotationDirection(player);

                if (player->gamemode == GAMEMODE_SHIP) player->velY *= 0.7f;

                flipOtherPlayer(state.currentPlayer);

                state.currentData.jumps++;

                player->ballRotationSpeed = -BALL_SLOW_ROTATION;
                
                if (player->gamemode == GAMEMODE_ROBOT) setAnimation(player, ROBOT_ANIM_JUMP_START);
                if (player->gamemode == GAMEMODE_SPIDER) setAnimation(player, SPIDER_ANIM_JUMP);

                player->onGround = false;
                player->onCeiling = false;
                player->inverseRotation = false;
                player->leftGround = true;
                player->bufferingState = BUFFER_END;
                player->ceilingInvTime = 0.5f;
                player->jumped = true;

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 0;
                    effect->def.colorG = 255 / 255.f;
                    effect->def.colorB = 0;
                }

                player->lastOrbPressed = obj;

                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case TOGGLE_ORB: {
            if (!GET_COLLIDED(obj)) addUseEffect(objects.x[obj], objects.y[obj], obj, &orbCollideEffect, getUseEffectArrayPtr(GFX_TOP));
            if (!GET_ACTIVATED(obj) && (currInput.holdJump) && (player->bufferingState == BUFFER_READY || currInput.pressedJump)) {
                ToggleTrigger *trigger = getToggleTrigger(obj);

                if (trigger->spawnOnly) spawnGroup(trigger->targetGroup);
                else toggleGroup(trigger->targetGroup, trigger->activateGroup, true);

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &orbUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = objects.detailColor[obj].r / 255.f;
                    effect->def.colorG = objects.detailColor[obj].g / 255.f;
                    effect->def.colorB = objects.detailColor[obj].b / 255.f;
                }

                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case BLUE_GRAVITY_PORTAL: {
            player->gravObjID = obj;
            if (!GET_ACTIVATED(obj)) {
                startGravitySweep(true);
                player->ceilingInvTime = CEILING_INVUL_TIME;
                if (player->upsideDown) {
                    if (player->gamemode != GAMEMODE_BALL && state.showPlayer) motionTrailResumeStroke(trail);
                    if (player->gamemode == GAMEMODE_WAVE) motionTrailAddWavePoint(waveTrail);
                    player->velY /= -2;
                    player->upsideDown = false;
                    player->inverseRotation = false;
                    player->robotAirTime = 999.f;

                    if (player->gamemode == GAMEMODE_UFO) 
                        player->rotation = 0;
                    else
                        player->snapRotation = true;

                    flipOtherPlayer(state.currentPlayer);
                    player->leftGround = true;
                    
                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 0 / 255.f;
                        effect->def.colorG = 200 / 255.f;
                        effect->def.colorB = 255 / 255.f;
                    }
                }
                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case YELLOW_GRAVITY_PORTAL: {
            player->gravObjID = obj;
            if (!GET_ACTIVATED(obj)) {
                startGravitySweep(false);
                player->ceilingInvTime = CEILING_INVUL_TIME;
                if (!player->upsideDown) {
                    if (player->gamemode != GAMEMODE_BALL && state.showPlayer) motionTrailResumeStroke(trail);
                    if (player->gamemode == GAMEMODE_WAVE) motionTrailAddWavePoint(waveTrail);
                    player->velY /= -2;
                    player->upsideDown = true;
                    player->inverseRotation = false;
                    player->robotAirTime = 999.f;

                    if (player->gamemode == GAMEMODE_UFO) 
                        player->rotation = 0;
                    else
                        player->snapRotation = true;
                        
                    flipOtherPlayer(state.currentPlayer);
                    player->leftGround = true;
                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 255 / 255.f;
                        effect->def.colorG = 200 / 255.f;
                        effect->def.colorB = 0;
                    }
                }
                SET_ACTIVATED(obj, true);
            } 
            break;
        }
        case ORANGE_MIRROR_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                if (state.intendedMirrorFactor != 1.f) {
                    motionTrailStopStroke(trail);
                    state.mirroring = true;
                    state.mirrorTimer = 0;
                    state.originalMirrorFactor = state.mirrorFactor;
                    state.intendedMirrorFactor = 1.f;
                }
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 150 / 255.f;
                    effect->def.colorB = 0 / 255.f;
                }
                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case BLUE_MIRROR_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                if (state.intendedMirrorFactor != 0.f) {
                    motionTrailStopStroke(trail);
                    state.mirroring = true;
                    state.mirrorTimer = 0;
                    state.originalMirrorFactor = state.mirrorFactor;
                    state.intendedMirrorFactor = 0.f;
                }
                
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 0 / 255.f;
                    effect->def.colorG = 255 / 255.f;
                    effect->def.colorB = 255 / 255.f;
                }

                SET_ACTIVATED(obj, true);
            }
            break;
        
        }
        case BIG_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                if (player->mini) {
                    startBgFlash();

                    lightningBolts.push_back(
                        Lightning(
                            objects.x[obj] + randomInt(-25.0f, 25.0f) + 25.0f, state.cameraY + 50.0f + SCREEN_HEIGHT, 
                            objects.x[obj], objects.y[obj], 
                            LightningType::SIZE_NORMAL
                        )
                    );

                    setMini(player, false);
                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 0 / 255.f;
                        effect->def.colorG = 255 / 255.f;
                        effect->def.colorB = 50 / 255.f;
                    }
                    UseEffect *effect2;
                    if (state.currentPlayer == 0) {
                        effect2 = addUseEffect(player1->x, player1->y, USE_EFFECT_OBJ_P1, &sizeChangeBigEffect, getUseEffectArrayPtr(GFX_TOP));
                    } else {
                        effect2 = addUseEffect(player2->x, player2->y, USE_EFFECT_OBJ_P2, &sizeChangeBigEffect, getUseEffectArrayPtr(GFX_TOP));
                    }                    

                    if (effect2) {
                        effect2->def.colorR = 0 / 255.f;
                        effect2->def.colorG = 255 / 255.f;
                        effect2->def.colorB = 50 / 255.f;
                    }
                }
                SET_ACTIVATED(obj, true);
            }
            break;        
        }
        case MINI_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                if (!player->mini){
                    startBgFlash();

                    lightningBolts.push_back(
                        Lightning(
                            objects.x[obj] + randomInt(-25.0f, 25.0f) + 25.0f, state.cameraY + 50.0f + SCREEN_HEIGHT, 
                            objects.x[obj], objects.y[obj], 
                            LightningType::SIZE_MINI
                        )
                    );

                    setMini(player, true);
                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 255 / 255.f;
                        effect->def.colorG = 31 / 255.f;
                        effect->def.colorB = 255 / 255.f;
                    }
                    if (state.currentPlayer == 0) {
                        UseEffect *effect2 = addUseEffect(player1->x, player1->y, USE_EFFECT_OBJ_P1, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                        if (effect2) {
                            effect2->def.colorR = 255 / 255.f;
                            effect2->def.colorG = 31 / 255.f;
                            effect2->def.colorB = 255 / 255.f;
                        }
                    } else {
                        UseEffect *effect2 = addUseEffect(player2->x, player2->y, USE_EFFECT_OBJ_P2, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                        if (effect2) {
                            effect2->def.colorR = 255 / 255.f;
                            effect2->def.colorG = 31 / 255.f;
                            effect2->def.colorB = 255 / 255.f;
                        }
                    }    
                }
                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case SLOW_SPEED_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                SET_ACTIVATED(obj, true);
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &speedCollideEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 255 / 255.f;
                    effect->def.colorB = 0 / 255.f;
                }
                if (state.speed != SPEED_SLOW) {
                    state.speed = SPEED_SLOW;
                    slowSpeedParticlesTimer = 0.7f;
                };
            }
            break;
        }
        case NORMAL_SPEED_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                SET_ACTIVATED(obj, true);
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &speedCollideEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 0 / 255.f;
                    effect->def.colorG = 190 / 255.f;
                    effect->def.colorB = 255 / 255.f;
                }
                if (state.speed != SPEED_NORMAL) {
                    state.speed = SPEED_NORMAL;
                    normalSpeedParticlesTimer = 0.7f;
                }
            }
            break;
        }
        case FAST_SPEED_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                SET_ACTIVATED(obj, true);
                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &speedCollideEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 0 / 255.f;
                    effect->def.colorG = 255 / 255.f;
                    effect->def.colorB = 0 / 255.f;
                }
                if (state.speed != SPEED_FAST){
                    state.speed = SPEED_FAST;
                    fastSpeedParticlesTimer = 0.7f;
                }
            }
            break;
        }
        case FASTER_SPEED_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                SET_ACTIVATED(obj, true);

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &speedCollideEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 230 / 255.f;
                    effect->def.colorG = 65  / 255.f;
                    effect->def.colorB = 255 / 255.f;
                }
                
                if (state.speed != SPEED_FASTER) {
                    state.speed = SPEED_FASTER;
                    fasterSpeedParticlesTimer = 0.7f;
                }
            }
            break;
        }
        case FASTEST_SPEED_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                SET_ACTIVATED(obj, true);

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &speedCollideEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 255 / 255.f;
                    effect->def.colorG = 65  / 255.f;
                    effect->def.colorB = 65  / 255.f;
                }

                if (state.speed != SPEED_FASTEST) {
                    state.speed = SPEED_FASTEST;
                    fastestSpeedParticlesTimer = 0.7f;
                }
            }
            break;
        }
        case CUBE_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                state.groundY = 0;
                state.ceilingY = 999999;
                if (player->gamemode != GAMEMODE_PLAYER) {
                    if (player->gamemode != GAMEMODE_BALL && player->gamemode != GAMEMODE_ROBOT) {
                        player->velY /= 2;
                    }

                    if (player->gamemode == GAMEMODE_WAVE) player->velY *= 0.9f;
                    
                    player->ceilingInvTime = GAMEMODE_INVUL_TIME;
                    player->snapRotation = true;
                    setGamemode(player, GAMEMODE_PLAYER);
                    setCheckpointTimer(0);
                    pseudoCheckpointExists = false;
                    flipOtherPlayer(state.currentPlayer ^ 1);
                    updateRotationDirection(player);
                }

                if (state.dual) {
                    setDualBounds();
                } 

                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case ROBOT_PORTAL: {
                if (!GET_ACTIVATED(obj)) {
                    state.groundY = 0;
                    state.ceilingY = 999999;
                    if (player->gamemode != GAMEMODE_ROBOT) {
                        if (player->gamemode != GAMEMODE_BALL && player->gamemode != GAMEMODE_PLAYER) {
                            player->velY /= 2;
                        }
                        if (player->gamemode == GAMEMODE_WAVE) player->velY *= 0.9f;

                        player->ceilingInvTime = GAMEMODE_INVUL_TIME;
                        player->snapRotation = true;
                        player->inverseRotation = false;
                        setGamemode(player, GAMEMODE_ROBOT);
                        setCheckpointTimer(0);
                        pseudoCheckpointExists = false;
                        flipOtherPlayer(state.currentPlayer ^ 1);
                        updateRotationDirection(player);

                        setAnimation(player, player->dashing ? ROBOT_ANIM_FALL_START : ROBOT_ANIM_JUMP_START);
                        player->robotAirTime = 999.f;

                        UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                        if (effect) {
                            effect->def.colorR = 255 / 255.f;
                            effect->def.colorG = 100 / 255.f;
                            effect->def.colorB = 0 / 255.f;
                        }
                    }

                    if (state.dual) {
                        setDualBounds();
                    } 

                    SET_ACTIVATED(obj, true);
                }
                break;
            }   
        case SPIDER_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                state.groundY = fmaxf(0, ip1Ceilf((objects.originalY[obj] - ((gamemodeHeights[GAMEMODE_SPIDER] + 60.f) / 2.f)) / 30.f)) * 30;
                state.ceilingY = state.groundY + gamemodeHeights[GAMEMODE_SPIDER];
                setIntendedCeiling();

                if (player->gamemode != GAMEMODE_SPIDER) {
                    if (player->gamemode != GAMEMODE_BALL && player->gamemode != GAMEMODE_PLAYER) {
                        player->velY /= 2;
                    }
                    if (player->gamemode == GAMEMODE_WAVE) player->velY *= 0.9f;

                    player->ceilingInvTime = GAMEMODE_INVUL_TIME;
                    player->snapRotation = true;
                    player->inverseRotation = false;
                    setGamemode(player, GAMEMODE_SPIDER);
                    setCheckpointTimer(0);
                    pseudoCheckpointExists = false;
                    flipOtherPlayer(state.currentPlayer ^ 1);
                    updateRotationDirection(player);

                    setAnimation(player, player->dashing ? SPIDER_ANIM_FALL : SPIDER_ANIM_RUN);

                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 255 / 255.f;
                        effect->def.colorG = 50 / 255.f;
                        effect->def.colorB = 50 / 255.f;
                    }
                }

                if (state.dual) {
                    setDualBounds();
                }

                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case BLUE_TP_PORTAL: {
            if (GET_ACTIVATED(obj)) break;
            int oi = objects.childObject[obj];
            if (oi >= 0) {
                lightningBolts.push_back(
                    Lightning(
                        objects.x[obj], objects.y[obj],
                        objects.x[oi], objects.y[oi], 
                        LightningType::TELEPORT
                    )
                );

                player->y = objects.y[oi];
                state.oldPlayer.y = player->y;

                float halfView = SCREEN_HEIGHT_AREA / 2.0f;
                float playerScreenY = player->y - (state.cameraY + halfView - LEVEL_Y_OFFSET);

                float upsideDownOffset = (player->upsideDown ? -30.f : 0);
                if (playerScreenY > halfView + 60.0f) {
                    // Top bound
                    setCameraY(player->y - (140.f + upsideDownOffset));
                } else if (playerScreenY < -(halfView + 60.0f)) {
                    // Bottom bound
                    setCameraY(player->y - (30.f + upsideDownOffset));
                }

                UseEffect *eff1 = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (eff1) {
                    eff1->def.colorR = 56 / 255.f;
                    eff1->def.colorG = 200 / 255.f;
                    eff1->def.colorB = 255 / 255.f;
                }

                UseEffect *eff2 = addUseEffect(objects.x[oi], objects.y[oi], oi, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (eff2) {
                    eff2->def.colorR = 255 / 255.f;
                    eff2->def.colorG = 255 / 255.f;
                    eff2->def.colorB = 0 / 255.f;
                }
            }
            SET_ACTIVATED(obj, true);
            player->justTeleported = true;
            motionTrailClear(trail);
            motionTrailClear(waveTrail);
            if (player->gamemode == GAMEMODE_WAVE) {
                waveTrail->positionR = (Vec2D){player->x, player->y};  
                waveTrail->startingPositionInitialized = true;
                motionTrailResumeStroke(waveTrail);
                motionTrailAddWavePoint(waveTrail);
            }
            break;
        }
        case SHIP_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                state.groundY = fmaxf(0, ip1Ceilf((objects.originalY[obj] - ((gamemodeHeights[GAMEMODE_SHIP] + 60.f) / 2.f)) / 30.f)) * 30;
                state.ceilingY = state.groundY + gamemodeHeights[GAMEMODE_SHIP];
                setIntendedCeiling();

                if (player->gamemode != GAMEMODE_SHIP) {
                    if (player->gamemode == GAMEMODE_WAVE) player->velY *= 0.9f;
                    player->velY /= (player->gamemode == GAMEMODE_UFO || player->gamemode == GAMEMODE_WAVE) ? 4 : 2;
                    
                    // No pseudo checkpoint if coming from non flying gamemode
                    if (!playerGamemodeIsFlying(player)) {
                        pseudoCheckpointExists = false;
                    }

                    setGamemode(player, GAMEMODE_SHIP);
                    setCheckpointTimer(AUTO_CHECKPOINT_TIME);
                    player->inverseRotation = false;
                    player->rotation = 0;
                    flipOtherPlayer(state.currentPlayer ^ 1);
                    
                    float min = player->mini ? -406.566f : -345.6f;
                    float max = player->mini ? 508.248f : 432.0f;

                    if (player->velY < min) {
                        player->velY = min;
                    } else if (player->velY > max) {
                        player->velY = max;
                    }
                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 255 / 255.f;
                        effect->def.colorG = 0 / 255.f;
                        effect->def.colorB = 255 / 255.f;
                    }
                }

                if (state.dual) {
                    setDualBounds();
                } 

                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case BALL_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                state.groundY = fmaxf(0, ip1Ceilf((objects.originalY[obj] - ((gamemodeHeights[GAMEMODE_BALL] + 60.f) / 2.f)) / 30.f)) * 30;
                state.ceilingY = state.groundY + gamemodeHeights[GAMEMODE_BALL];
                setIntendedCeiling();

                if (player->gamemode != GAMEMODE_BALL) {
                    player->ballRotationSpeed = -BALL_SLOW_ROTATION;

                    switch (player->gamemode) {
                        case GAMEMODE_WAVE:
                            player->velY *= 0.9f;
                            player->velY /= 2;
                        case GAMEMODE_SHIP:
                        case GAMEMODE_UFO:
                            player->velY /= 2;
                            break;
                    }
                    setGamemode(player, GAMEMODE_BALL);
                    setCheckpointTimer(0);
                    pseudoCheckpointExists = false;

                    if (currInput.holdJump && state.oldPlayer.gamemode == GAMEMODE_SHIP) {
                        player->bufferingState = BUFFER_READY;
                    }

                    player->inverseRotation = false;
                    player->snapRotation = true;
                    flipOtherPlayer(state.currentPlayer ^ 1);
                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorG = 50 / 255.f;
                        effect->def.colorB = 50 / 255.f;
                        effect->def.colorR = 255 / 255.f;
                    }
                }

                if (state.dual) {
                    setDualBounds();
                } 
                
                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case UFO_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                state.groundY = fmaxf(0, ip1Ceilf((objects.originalY[obj] - ((gamemodeHeights[GAMEMODE_UFO] + 60.f) / 2.f)) / 30.f)) * 30;
                state.ceilingY = state.groundY + gamemodeHeights[GAMEMODE_UFO];
                setIntendedCeiling();
                
                if (player->gamemode != GAMEMODE_UFO) {
                    if (player->gamemode == GAMEMODE_WAVE) player->velY *= 0.9f;
                    player->velY /= (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_WAVE) ? 4 : 2;
                    
                    // No pseudo checkpoint if coming from non flying gamemode
                    if (!playerGamemodeIsFlying(player)) {
                        pseudoCheckpointExists = false;
                    }

                    setGamemode(player, GAMEMODE_UFO);
                    setCheckpointTimer(AUTO_CHECKPOINT_TIME);
                    player->inverseRotation = false;
                    player->rotation = 0;
                    flipOtherPlayer(state.currentPlayer ^ 1);

                    if (state.oldPlayer.gamemode == GAMEMODE_SHIP || state.oldPlayer.gamemode == GAMEMODE_WAVE) {
                        player->bufferingState = BUFFER_READY;
                    }

                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 255 / 255.f;
                        effect->def.colorG = 150 / 255.f;
                        effect->def.colorB = 0 / 255.f;
                    }
                }
                if (state.dual) {
                    setDualBounds();
                } 

                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case WAVE_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                state.groundY = fmaxf(0, ip1Ceilf((objects.originalY[obj] - ((gamemodeHeights[GAMEMODE_WAVE] + 60.f) / 2.f)) / 30.f)) * 30;
                state.ceilingY = state.groundY + gamemodeHeights[GAMEMODE_WAVE];
                setIntendedCeiling();

                if (player->gamemode != GAMEMODE_WAVE) {
                    // No pseudo checkpoint if coming from non flying gamemode
                    if (!playerGamemodeIsFlying(player)) {
                        pseudoCheckpointExists = false;
                    }
                    
                    setGamemode(player, GAMEMODE_WAVE);
                    setCheckpointTimer(AUTO_CHECKPOINT_TIME);
                    player->inverseRotation = false;
                    player->snapRotation = true;
                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 255 / 255.f;
                        effect->def.colorG = 200 / 255.f;
                        effect->def.colorB = 0 / 255.f;
                    }
                    UseEffect *effect2 = addUseEffect(objects.x[obj], objects.y[obj], obj, &waveRadiusEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect2) {
                        effect2->def.colorR = getWhiteIfBlack(p1Color).r;
                        effect2->def.colorG = getWhiteIfBlack(p1Color).g;
                        effect2->def.colorB = getWhiteIfBlack(p1Color).b;
                    }
                }

                waveTrail->positionR = (Vec2D){player->x, player->y};  
                waveTrail->startingPositionInitialized = true;
                
                motionTrailResumeStroke(waveTrail);
                motionTrailAddWavePoint(waveTrail);

                if (state.dual) {
                    setDualBounds();
                }

                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case DUAL_PORTAL: {
            player->gravObjID = obj;
            if (!GET_ACTIVATED(obj)) {
                if (!state.dual){
                    player->ceilingInvTime = CEILING_INVUL_TIME;
                    state.dual = true;
                    state.dualPortalY = objects.originalY[obj];
                    setupDual();
                    if (state.currentPlayer == 0) state.player2.x = state.oldPlayer.x; // Sync them
                }
                setDualBounds();

                motionTrailClear(&trailP2);
                if (state.player2.gamemode == GAMEMODE_WAVE) {
                    waveTrailP2.positionR = (Vec2D){state.player2.x, state.player2.y};  
                    waveTrailP2.startingPositionInitialized = true;
                    motionTrailClear(&waveTrailP2);
                    
                    motionTrailResumeStroke(&waveTrailP2);
                    motionTrailAddWavePoint(&waveTrailP2);
                }
                SET_ACTIVATED(obj, true);                
            }
            break;
        }
        case DIVORCE_PORTAL: {
            if (!GET_ACTIVATED(obj)) {
                if (state.dual){
                    state.dual = false;
                    if (state.currentPlayer == 1) {
                        memcpy(&state.player, player, sizeof(Player));

                        motionTrailCopyTrail(&trailP1, &trailP2);
                        motionTrailCopyTrail(&waveTrailP1, &waveTrailP2);   
                    }
                    switch (state.player.gamemode) {
                        case GAMEMODE_PLAYER:
                        case GAMEMODE_ROBOT:
                            state.groundY = 0;
                            state.ceilingY = 999999;
                            break;
                        case GAMEMODE_SHIP:
                        case GAMEMODE_UFO:
                        case GAMEMODE_BALL:
                        case GAMEMODE_SPIDER:
                            state.groundY = fmaxf(0, ip1Ceilf((state.dualPortalY - ((gamemodeHeights[state.player.gamemode] + 60.f) / 2.f)) / 30.f)) * 30;
                            state.ceilingY = state.groundY + gamemodeHeights[state.player.gamemode];
                            setIntendedCeiling();
                    }
                    
                    motionTrailStopStroke(&trailP2);
                    motionTrailStopStroke(&waveTrailP2);
                    
                    UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &portalUseEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effect) {
                        effect->def.colorR = 145 / 255.f;
                        effect->def.colorG = 255 / 255.f;
                        effect->def.colorB = 255 / 255.f;
                    }
                    
                }
                SET_ACTIVATED(obj, true);
            }
            break;
        }
        case SECRET_COIN: {
            if (!GET_ACTIVATED(obj)) {
                SET_ACTIVATED(obj, true);

                bool isCollected = isCoinCollected(obj);

                if (state.practiceMode) break;

                UseEffect *effect = addUseEffect(objects.x[obj], objects.y[obj], obj, &coinUseEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    effect->def.colorR = 1.f;
                    effect->def.colorG = state.customLevel ? 1.f : 190 / 255.f;
                    effect->def.colorB = state.customLevel ? 1.f : 0 / 255.f;
                }

                UseEffect *effect2 = addUseEffect(objects.x[obj], objects.y[obj], obj, &coinRadiusEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effect2) {
                    effect2->def.colorR = 1.f;
                    effect2->def.colorG = state.customLevel ? 1.f : 190 / 255.f;
                    effect2->def.colorB = state.customLevel ? 1.f : 0 / 255.f;
                }
                coinPickupParticles.emitterX = objects.x[obj];
                coinPickupParticles.emitterY = objects.y[obj];
                spawnMultipleParticles(&coinPickupParticles, 40);

                
                startCollectEffect(objects.x[obj], objects.y[obj], isCollected);
                
                int coinID = objects.coinID[obj];
                switch (coinID) {
                    case 0:
                        state.currentData.coin1 = true;
                        break;
                    case 1:
                        state.currentData.coin2 = true;
                        break;
                    case 2:
                        state.currentData.coin3 = true;
                        break;
                }

                objects.flags[obj] |= FLAG_TOGGLED;
            }
            break;
        }
        case S_MODIFIERBLOCK:
            if (player->dashing) {
                stopDashing(player);
                player->holdingJumpCancelled = true;
            }
            break;

        case D_MODIFIERBLOCK:
            player->allowDartSlide = true;
            break;

        case J_MODIFIERBLOCK:
            if (player->lastOrbPressed >= 0) player->holdingJumpCancelled = true;
            break;

        case H_MODIFIERBLOCK:
            player->allowHeadHit = true;
            break;

        case 1329:
        case 1816:
            break;

    }
    if (!GET_COLLIDED(obj)) SET_HITBOX_COUNTER(obj, GET_HITBOX_COUNTER(obj) + 1); 
}

void getCorners(float cx, float cy, float w, float h, float angle, Vec2D out[4]) {
    float hw = w * 0.5f, hh = h * 0.5f;
    angle = -angle;
    float rad = C3D_AngleFromDegrees(angle);
    float cosA = cosf(rad), sinA = sinf(rad);
    
    // Precompute rotated half-dimensions
    float hwCos = hw * cosA, hwSin = hw * sinA;
    float hhCos = hh * cosA, hhSin = hh * sinA;
    
    out[0].x = cx - hwCos + hhSin;
    out[0].y = cy - hwSin - hhCos;
    
    out[1].x = cx + hwCos + hhSin;
    out[1].y = cy + hwSin - hhCos;
    
    out[2].x = cx + hwCos - hhSin;
    out[2].y = cy + hwSin + hhCos;
    
    out[3].x = cx - hwCos - hhSin;
    out[3].y = cy - hwSin + hhCos;
}

static bool satOverlapExclusive(const Vec2D a[4], const Vec2D b[4]) {
    // Test all axes (normals of edges)
    for (int shape = 0; shape < 2; ++shape) {
        const Vec2D *verts = (shape == 0) ? a : b;
        for (int i = 0; i < 4; ++i) {
            // Edge from verts[i] to verts[(i+1)%4]
            float dx = verts[(i+1)%4].x - verts[i].x;
            float dy = verts[(i+1)%4].y - verts[i].y;
            // Normal axis
            float ax = -dy, ay = dx;

            // Project both shapes onto axis
            float minA = INFINITY, maxA = -INFINITY;
            float minB = INFINITY, maxB = -INFINITY;
            for (int j = 0; j < 4; ++j) {
                float projA = a[j].x * ax + a[j].y * ay;
                float projB = b[j].x * ax + b[j].y * ay;
                if (projA < minA) minA = projA;
                if (projA > maxA) maxA = projA;
                if (projB < minB) minB = projB;
                if (projB > maxB) maxB = projB;
            }
            // If projections do not overlap, there is a separating axis
            if (maxA <= minB || maxB <= minA) return false;
        }
    }
    return true;
}

bool intersectExclusive(float x1, float y1, float w1, float h1, float angle1,
                    float x2, float y2, float w2, float h2, float angle2) {
    // Tighter AABB check
    float maxExtent = fmaxf(w1, h1) + fmaxf(w2, h2);
    float dx = fabsf(x1 - x2);
    float dy = fabsf(y1 - y2);
    
    if (dx > maxExtent || dy > maxExtent) {
        return false;
    }
    
    Vec2D rect1[4], rect2[4];
    getCorners(x1, y1, w1, h1, angle1, rect1);
    getCorners(x2, y2, w2, h2, angle2, rect2);
    return satOverlapExclusive(rect1, rect2);
}

static bool satOverlap(const Vec2D a[4], const Vec2D b[4]) {
    // Test all axes (normals of edges)
    for (int shape = 0; shape < 2; ++shape) {
        const Vec2D *verts = (shape == 0) ? a : b;
        for (int i = 0; i < 4; ++i) {
            // Edge from verts[i] to verts[(i+1)%4]
            float dx = verts[(i+1)%4].x - verts[i].x;
            float dy = verts[(i+1)%4].y - verts[i].y;
            // Normal axis
            float ax = -dy, ay = dx;

            // Project both shapes onto axis
            float minA = INFINITY, maxA = -INFINITY;
            float minB = INFINITY, maxB = -INFINITY;
            for (int j = 0; j < 4; ++j) {
                float projA = a[j].x * ax + a[j].y * ay;
                float projB = b[j].x * ax + b[j].y * ay;
                if (projA < minA) minA = projA;
                if (projA > maxA) maxA = projA;
                if (projB < minB) minB = projB;
                if (projB > maxB) maxB = projB;
            }
            // If projections do not overlap, there is a separating axis
            if (maxA < minB || maxB < minA) return false;
        }
    }
    return true;
}

bool intersect(float x1, float y1, float w1, float h1, float angle1,
                    float x2, float y2, float w2, float h2, float angle2) {
    // Tighter AABB check
    float maxExtent = fmaxf(w1, h1) + fmaxf(w2, h2);
    float dx = fabsf(x1 - x2);
    float dy = fabsf(y1 - y2);
    
    if (dx > maxExtent || dy > maxExtent) {
        return false;
    }
    
    Vec2D rect1[4], rect2[4];
    getCorners(x1, y1, w1, h1, angle1, rect1);
    getCorners(x2, y2, w2, h2, angle2, rect2);
    return satOverlap(rect1, rect2);
}

bool intersectRectCircle(float rx, float ry, float rw, float rh, float rangle,
                          float cx, float cy, float cradius) {
    // If centers are too far apart, no collision
    float maxDim = fmaxf(rw, rh);
    float maxDist = (maxDim / 2.0f) + cradius;
    if (fabsf(rx - cx) > maxDist || fabsf(ry - cy) > maxDist) {
        return false;
    }

    // Transform circle center into rectangle's local space
    float rad = -C3D_AngleFromDegrees(rangle); // negative for inverse rotation
    float cosA = cosf(rad), sinA = sinf(rad);

    float localCx = cosA * (cx - rx) - sinA * (cy - ry) + rx;
    float localCy = sinA * (cx - rx) + cosA * (cy - ry) + ry;

    // Rectangle bounds
    float left   = rx - rw / 2.0f;
    float right  = rx + rw / 2.0f;
    float top    = ry - rh / 2.0f;
    float bottom = ry + rh / 2.0f;

    // Find closest point on rectangle to circle center
    float closestX = fmaxf(left, fminf(localCx, right));
    float closestY = fmaxf(top,  fminf(localCy, bottom));

    // Distance from circle center to closest point
    float dx = localCx - closestX;
    float dy = localCy - closestY;
    float distSq = dx * dx + dy * dy;

    return distSq <= cradius * cradius;
}
// Dot product helper
float dot(float x1, float y1, float x2, float y2) {
    return x1 * x2 + y1 * y2;
}

bool circleRectCollision(float cx, float cy, float radius,
                           float x1, float y1, float x2, float y2) {
    // Vector from point 1 to circle center
    float dx = x2 - x1;
    float dy = y2 - y1;
    float fx = cx - x1;
    float fy = cy - y1;

    float lenSq = dx * dx + dy * dy;
    float t = dot(fx, fy, dx, dy) / lenSq;

    // Clamp t to the [0,1] range to stay within the segment
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;

    // Closest point on segment
    float closestX = x1 + t * dx;
    float closestY = y1 + t * dy;

    // Distance from circle center to closest point
    float distX = cx - closestX;
    float distY = cy - closestY;
    float distSq = distX * distX + distY * distY;

    return distSq <= radius * radius;
}

static float movingSurfaceVelocity(Player *player, int obj) {
    if (moveStepDt <= 0.f) return 0.f;
    return grav(player, objects.y[obj] - objects.lastY[obj]) / moveStepDt;
}

static bool movingSupportCrossed(Player *player, int obj, float velocity) {
    if (velocity <= MINIMUM_OBJECT_SPEED || player->gamemode == GAMEMODE_WAVE ||
        player->upsideDown != state.oldPlayer.upsideDown || player->justTeleported) return false;
    float oldTop = grav(player, objects.lastY[obj]) + objects.height[obj] * .5f;
    float newTop = objGravTop(player, obj);
    return gravBottom(&state.oldPlayer) >= oldTop - .001f &&
        gravBottom(player) <= newTop && player->velY <= velocity;
}

static void finishMovingContact(Player *player) {
    bool reset = state.dead || player->upsideDown != state.oldPlayer.upsideDown || player->justTeleported ||
        player->gamemode != state.oldPlayer.gamemode || player->gamemode == GAMEMODE_WAVE ||
        player->slopeData.slopeID >= 0;
    float launch;
    if (!reset && platformLaunch(grav(player, player->lastPlatformVelocity), grav(player, player->platformVelocity),
        player->velY, STEPS_DT, &launch)) {
        player->velY = launch;
        player->velocityOverride = true;
        player->onGround = player->onCeiling = false;
        player->leftGround = true;
        player->inverseRotation = player->gamemode == GAMEMODE_PLAYER;
        if (state.showPlayer) motionTrailResumeStroke(trail);
    }
    player->lastPlatformVelocity = reset ? 0.f : player->platformVelocity;
}

void handleCollision(Player *player, int obj, const ObjectHitbox *hitbox) {
    InternalHitbox internal = player->internalHitbox;

    float clip = (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_UFO) ? 7 : 10;
    switch (hitbox->collisionType) {
        //case HITBOX_BREAKABLE_BLOCK:
        case HITBOX_SOLID: {
            bool gravSnap = false;

            float x = objects.x[obj];
            float y = objects.y[obj];
            float objectRawVel = movingSurfaceVelocity(player, obj);
            bool movingSupport = movingSupportCrossed(player, obj, objectRawVel);

            // This is equal to using the old player y position (a frame of snap leeway)
            clip += fabsf(player->velY) * STEPS_DT;

            bool modeSnap = state.oldPlayer.gamemode != player->gamemode && state.oldPlayer.gamemode == GAMEMODE_WAVE && player->allowDartSlide;

            if (modeSnap) clip += (player->height - state.oldPlayer.height) / 2;
            
            if (player->ceilingInvTime > 0) {
                // Only do the funny grav snap if player is touching a gravity object and internal hitbox is touching block
                bool internalCollidingBlock = intersect(
                    player->x, player->y, internal.width, internal.height, 0, 
                    x, y, objects.width[obj], objects.height[obj], objects.rotation[obj]
                );

                float diff = objGravBottom(player, obj) - gravInternalBottom(player);

                gravSnap = internalCollidingBlock && (diff >= 0 && diff <= clip);
            }

            bool safeZone = player->mini && ((objGravTop(player, obj) - gravBottom(player) <= clip) || (gravTop(player) - objGravBottom(player, obj) <= clip));
            
            bool dartSlide = player->gamemode == GAMEMODE_WAVE && player->allowDartSlide &&
                objects.id[obj] != BREAKABLE_BLOCK &&
                ((((objGravTop(player, obj) - gravBottom(player) <= clip) || movingSupport) && player->velY <= fmaxf(objectRawVel, 0)) ||
                ((gravTop(player) - objGravBottom(player, obj) <= clip) && player->velY >= objectRawVel));

            bool landFace = ((objGravTop(player, obj) - gravBottom(player) <= clip) || movingSupport) && player->velY <= fmaxf(objectRawVel, 0);
            bool ceilFace = (gravTop(player) - objGravBottom(player, obj) <= clip) && player->velY >= objectRawVel;
            float snapGap = landFace ? (objGravTop(player, obj) - gravBottom(player)) : (gravTop(player) - objGravBottom(player, obj));
            bool snapGrace = modeSnap && (landFace || ceilFace) && snapGap > 0.f && snapGap <= player->height / 2;

            // Check collision with internal hitbox
            if (!dartSlide && !snapGrace && (player->gamemode == GAMEMODE_WAVE || (!gravSnap && !safeZone && !movingSupport)) && intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                x, y, objects.width[obj], objects.height[obj], 0
            )) {
                if (objects.id[obj] == BREAKABLE_BLOCK) {
                    // Spawn breakable brick particles
                    brickDestroyParticles.emitterX = x;
                    brickDestroyParticles.emitterY = y;
                    spawnMultipleParticles(&brickDestroyParticles, 25);
                    objects.flags[obj] |= FLAG_TOGGLED;
                } else {
                    // Not a brick, die
                    killPlayer(DEATH_BLOCK);
                }

                return;
            }
            
            float bottom = gravBottom(player);

            // If you are on a slope, check more stuff (this is less shit than before)
            if (player->slopeData.slopeID >= 0) {
                int orient = gravSlopeOrient(player->slopeData.slopeID, player);

                // Down slope must never exit
                bool downSlope = (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN);
                if (!downSlope) {
                    return;
                }
                // Check if block should collide with player, then bye bye slope
                if (objGetRight(obj) >= player->x) {
                    clearSlopeData(player);
                    clearSlopeData(&state.oldPlayer);
                    clearCoyoteSlopeData(player);
                    clearCoyoteSlopeData(&state.oldPlayer);
                    player->onGround = true;
                }
            }

            // Check for slopes so this block doesn't catch up a down going slope  
            for (int i = 0; i < potentialSlopes[state.currentPlayer]; i++) {
                int potentialSlope = potentialSlopesBuffer[state.currentPlayer][i];

                unsigned char orient = objects.orientation[potentialSlope];
                float blockComp = orient < 2 ? objGetTop(obj) : objGetBottom(obj);
                float slopeComp = orient < 2 ? objGetBottom(potentialSlope) : objGetTop(potentialSlope);

                if (blockComp - slopeComp < 2) {
                    return;
                }
            }

            // Check snap for player bottom
            if ((objGravTop(player, obj) - bottom <= clip || movingSupport) && player->velY <= fmaxf(objectRawVel, 0) && (player->gamemode != GAMEMODE_WAVE || player->allowDartSlide)) {
                player->y = grav(player, objGravTop(player, obj)) + grav(player, player->height / 2);
                if (player->velY <= 0) player->velY = 0;
                player->onGround = true;
                player->inverseRotation = false;
                player->timeSinceGround = 0;
                playerNonFlyingLanding(player);

                player->collidedBlock = obj;
                player->platformVelocity = grav(player, fmaxf(0.f, objectRawVel) * .9f);
                if (objectRawVel > MINIMUM_OBJECT_SPEED)
                    player->velY = fmaxf(player->velY, objectRawVel * .9f);

                if (player->gamemode == GAMEMODE_PLAYER) {
                    // Check for x snap
                    if (!state.oldPlayer.onGround) {
                        if (player->snapData.playerFrame > 0 && player->snapData.playerFrame + 1 < player->frame) {
                            trySnap(obj, player);
                        }
                    }

                    player->snapData.playerFrame = levelFrame;
                    player->snapData.objectID = obj;
                    player->snapData.playerSnapDiff = player->x - x;
                }
            // Check snap for player top
            } else if (player->gamemode != GAMEMODE_WAVE || player->allowDartSlide) {
                // Ufo can break breakable blocks from above, so dont use as a ceiling
                if (player->gamemode == GAMEMODE_UFO && objects.id[obj] == BREAKABLE_BLOCK) {
                    break;
                }
                // Behave normally
                if ((player->gamemode != GAMEMODE_PLAYER && player->gamemode != GAMEMODE_ROBOT && player->gamemode != GAMEMODE_SPIDER) || gravSnap || player->allowHeadHit) {
                    if (((gravTop(player) - objGravBottom(player, obj) <= clip && player->velY >= objectRawVel) || gravSnap)) {
                        if (!gravSnap) player->onCeiling = true;
                        else player->velY = 0;
                        player->inverseRotation = false;
                        player->timeSinceGround = 0;
                        player->y = grav(player, objGravBottom(player, obj)) - grav(player, player->height / 2);
                        if (player->velY >= 0) player->velY = 0;
                    }
                }
            }
            break;
        }
        case HITBOX_HAZARD: {
            killPlayer(hitbox->type == COLLISION_CIRCLE ? DEATH_SAW : DEATH_SPIKE);
            break;
        }
        case HITBOX_SPECIAL: {
            handleSpecialHitbox(player, obj, hitbox);
            break;
        }
    }
}

void handleMovingBlock(Player *player, int obj) {
    if (obj < 0 || obj >= objects.count || objects.flags[obj] & FLAG_TOGGLED ||
        !state.oldPlayer.onGround || player->velY > 0.f || player->justTeleported ||
        player->upsideDown != state.oldPlayer.upsideDown) return;
    if (fabsf(player->x - objects.x[obj]) > (player->width + objects.width[obj]) * .5f) return;
    float velocity = movingSurfaceVelocity(player, obj);
    if (velocity < 0.f && velocity >= -MINIMUM_OBJECT_SPEED)
        player->y += objects.y[obj] - objects.lastY[obj];
}

void collideWithObj(Player *player, int obj) {
    int objID = objects.id[obj];
    const ObjectHitbox *hitbox = gameObjects[objID].hitbox;

    if (!hitbox || objects.flags[obj] & FLAG_TOGGLED) return;

    snapshot.collisionChecks++;

    // hitbox offset by object rotation
    float angle = -objects.rotation[obj];
    float rot_rad = C3D_AngleFromDegrees(adjustAngleY(angle, objects.flippedH[obj]));
    float cosR = cosf(rot_rad), sinR = sinf(rot_rad);

    float rawOffX = hitbox->x * objects.scaleX[obj];
    float rawOffY = hitbox->y * objects.scaleY[obj];

    float offX = rawOffX * cosR - rawOffY * sinR;
    float offY = rawOffX * sinR + rawOffY * cosR;
    
    float x = objects.x[obj] + offX;
    float y = objects.y[obj] + offY;
    float width = objects.width[obj];
    float height = objects.height[obj];

    if (UNLIKELY(hitbox->type == COLLISION_CIRCLE)) {
        if (intersectRectCircle(
            player->x, player->y, player->width, player->height, 0, 
            x, y, objects.width[obj]
        )) {
            handleCollision(player, obj, hitbox);
            SET_COLLIDED(obj, true);
            snapshot.collisions++;
        } else {
            SET_COLLIDED(obj, false);
        }
    } else {
        float objRot = normalizeAngle(objects.rotation[obj]);

        // No rotation for solid hitboxes
        if (hitbox->collisionType == HITBOX_SOLID) {
            objRot = 0;
        }

        float rotation = (objRot == 0 || objRot == 90 || objRot == 180 || objRot == 270) ? 0 : player->rotation;
        
        bool checkColl = intersect(
            player->x, player->y, player->width, player->height, rotation, 
            x, y, width, height, objRot
        );

        // Rotated hitboxes must also collide with the unrotated hitbox
        if (rotation != 0) {
            checkColl = checkColl && intersect(
                player->x, player->y, player->width, player->height, 0, 
                x, y, width, height, objRot
            );
        }

        if (checkColl) {
            handleCollision(player, obj, hitbox);
            SET_COLLIDED(obj, true);
            snapshot.collisions++;
        } else {
            SET_COLLIDED(obj, false);
        }
    }
}

void collideWithSlope(Player *player, int obj, bool hasSlope) {
    const ObjectHitbox *hitbox = gameObjects[objects.id[obj]].hitbox;
    
    if (!hitbox || objects.flags[obj] & FLAG_TOGGLED) return;
    
    float width = objects.width[obj];
    float height = objects.height[obj];

    if (intersectExclusive(
        player->x, player->y, player->width, player->height, 0, 
        objects.x[obj], objects.y[obj], width, height, 0
    )) {
        slopeCollide(obj, player);
    }
}

int slopeBuffer[MAX_COLLIDED_OBJECTS];
int slopeCount = 0;

int blockBuffer[MAX_COLLIDED_OBJECTS];
int blockCount = 0;

int hazardBuffer[MAX_COLLIDED_OBJECTS];
int hazardCount = 0;

int potentialSlopesBuffer[2][MAX_COLLIDED_OBJECTS];
int potentialSlopes[2];

void collideWithObjects(Player *player) {
    player->platformVelocity = 0.f;
    player->lastCollidedBlock = player->collidedBlock;
    player->collidedBlock = -1;

    if (player->lastCollidedBlock >= 0) {
        handleMovingBlock(player, player->lastCollidedBlock);
    }
    
    int sx = (int)(player->x / SECTION_SIZE);
    int sy = (int)(player->y / SECTION_SIZE);
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            Section *sec = getSection(sx + dx, sy + dy);
            for (int i = 0; i < sec->objectCount; i++) {
                int obj = sec->objects[i];

                // Skip invalid objects
                if (!isValidObject(objects.id[obj])) continue;

                const ObjectHitbox *hitbox = gameObjects[objects.id[obj]].hitbox;

                if (!hitbox) continue;
                
                // Save some types to buffer, so they can be checked in a type order
                if (hitbox->collisionType == HITBOX_SOLID) {
                    if (hitbox->type == COLLISION_SLOPE) {
                        if (slopeCount < MAX_COLLIDED_OBJECTS) {
                            slopeBuffer[slopeCount++] = obj;
                        }
                    } else {
                        if (blockCount < MAX_COLLIDED_OBJECTS) {
                            blockBuffer[blockCount++] = obj;
                        }
                    }
                } else if (hitbox->collisionType == HITBOX_HAZARD) {
                    if (hazardCount < MAX_COLLIDED_OBJECTS) {
                        hazardBuffer[hazardCount++] = obj;
                    }
                } else { // HITBOX_SPECIAL
                    collideWithObj(player, obj);
                }
            }
        }
    }

    if (player->leftGround) {
        clearSlopeData(player);
    }

    // Check blocks
    for (int i = 0; i < blockCount; i++) {
        int obj = blockBuffer[i];
        collideWithObj(player, obj);
    }
    potentialSlopes[state.currentPlayer] = 0;

    // Check slopes
    bool hasSlope = player->slopeData.slopeID >= 0;
    for (int i = 0; i < slopeCount; i++) {
        int obj = slopeBuffer[i];
        collideWithSlope(player, obj, hasSlope);
    }
    
    finishMovingContact(player);
    // Check hazards (spikes, saws)
    for (int i = 0; i < hazardCount; i++) {
        int obj = hazardBuffer[i];
        collideWithObj(player, obj);
    }

    player->touchingSlope = false;
    slopeCount = 0;
    blockCount = 0;
    hazardCount = 0;
}