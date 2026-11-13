#include "rays.hpp"
#include "math_helpers.hpp"
#include "main.hpp"
#include "state.hpp"
#include "utils/gfx.hpp"
#include "easing.hpp"

typedef struct {
    Vec2D position;
    
    float startDelay;
    float timer;

    float targetH;
    float tweenDuration;
    float startAlpha;
    float angle;

    float fadeTimer;
} RayData;

static float anglesArray[BEAM_COUNT];
static RayData rayData[BEAM_COUNT];

static bool started = false;
static bool fading = false;

static int currentRay = 0;
static float timer = 0;

static void prepareAnglesArray() {
    // Fill array
    for (int i = 0; i < BEAM_COUNT; i++) {
        anglesArray[i] = normalizeAngle(ANGLE_START + i * ANGLE_STEP);
    }

    // Swap around
    for (int i = BEAM_COUNT - 1; i > 0; i--) {
        int j = randomInt(0, i);
        float temp = anglesArray[i];
        anglesArray[i] = anglesArray[j];
        anglesArray[j] = temp;
    }
}

static void prepareRays() {
    for (int i = 0; i < BEAM_COUNT; i++) {
        rayData[i].position.x = levelInfo.wallX;
        rayData[i].position.y = levelInfo.wallY;

        rayData[i].startDelay = i * STAGGER_STEP + STAGGER_BASE + STAGGER_RAND * randomFloat(-1, 1);
        rayData[i].timer = 0;

        rayData[i].targetH = W_MID + W_JITTER * randomFloat(-1, 1);
        rayData[i].tweenDuration = DUR_BASE + DUR_JITTER * randomFloat(-1, 1);
        rayData[i].startAlpha = CLAMP(ALPHA_BASE + ALPHA_JITTER * randomFloat(-1, 1), 0, 1);
        rayData[i].angle = normalizeAngle(anglesArray[i] + ANGLE_STEP * randomf() + 90);

        rayData[i].fadeTimer = 0;
    }
}

static void drawRay(float x, float y, float length, float startWidth, float endWidth, float angle, float t, u32 color) {
    // Clamp interpolation factor
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float effectiveLength = length * t;

    float currentEndWidth = startWidth + (endWidth - startWidth) * t;

    float cosA = cosf(DegToRad(angle));
    float sinA = sinf(DegToRad(angle));

    Vec2D verts[4];

    // Start side
    verts[0].x = x;
    verts[0].y = y + startWidth * 0.5f;

    verts[1].x = x;
    verts[1].y = y - startWidth * 0.5f;

    // End side
    verts[2].x = x - effectiveLength;
    verts[2].y = y + currentEndWidth * 0.5f;

    verts[3].x = x - effectiveLength;
    verts[3].y = y - currentEndWidth * 0.5f;

    // Rotate + transform to screen space
    for (int i = 0; i < 4; i++) {
        float vx = verts[i].x - x;
        float vy = verts[i].y - y;

        float rx = vx * cosA - vy * sinA;
        float ry = vx * sinA + vy * cosA;

        verts[i].x = rx + x;
        verts[i].y = ry + y;

        verts[i].x = verts[i].x - state.cameraX;

        verts[i].y = SCREEN_HEIGHT - (verts[i].y - state.cameraY);

        verts[i].x = getMirrorX(verts[i].x, state.mirrorFactor);
    }

    // Submit quad
    C2D_DrawTriangle(
        verts[0].x, verts[0].y, color,
        verts[1].x, verts[1].y, color,
        verts[2].x, verts[2].y, color,
        0.0f);

    C2D_DrawTriangle(
        verts[2].x, verts[2].y, color,
        verts[1].x, verts[1].y, color,
        verts[3].x, verts[3].y, color,
        0.0f);
}

void raysStart() {
    started = true;
    fading = false;
    currentRay = 0;
    timer = 0;
    prepareAnglesArray();
    prepareRays();
}

void raysStartFade() {
    fading = true;
    
    for (int i = 0; i < BEAM_COUNT; i++) {
        rayData[i].fadeTimer = 0;
    }
}

void updateRays(float delta) {
    if (!started) return;
    
    for (int i = 0; i < BEAM_COUNT; i++) {
        RayData *data = &rayData[i];

        // Check if it should start
        if (data->startDelay < timer) {
            if (fading) {
                data->fadeTimer += delta;
            }
            data->timer += delta;
        }
    }

    timer += delta;

    // Remove rays
    if (rayData[0].fadeTimer > RAY_FADE_DURATION) {
        started = false;
        fading = false;
    }
}

void drawRays() {
    if (!started) return;

    for (int i = 0; i < BEAM_COUNT; i++) {
        RayData *data = &rayData[i];

        // Check if it should start
        if (data->startDelay < timer) {
            float x = data->position.x + 30;
            float y = data->position.y;
            
            float startWidth = data->targetH / 4;
            float endWidth = data->targetH;

            const float length = MAX_BEAM_W;

            float elapsed = data->timer;
            float duration = data->tweenDuration;

            float alpha = data->startAlpha * 255;

            if (fading) {
                alpha = easeValue(EASE_LINEAR, alpha, 0, data->fadeTimer, RAY_FADE_DURATION, 1);
            }

            Color col = getP2IfBlack(p1Color);

            u32 color = C2D_Color32(col.r, col.g, col.b, alpha);

            float t = easeValue(QUAD_OUT, 0, 1, elapsed, duration, 1);

            drawRay(x, y, length, startWidth, endWidth, data->angle, t, color);
        }
    }
}