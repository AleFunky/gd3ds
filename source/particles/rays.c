#include "rays.h"
#include "math_helpers.h"
#include "main.h"
#include "state.h"
#include "utils/gfx.h"
#include "easing.h"

typedef struct {
    Vec2D position;
    
    float start_delay;
    float timer;

    float target_h;
    float tween_duration;
    float start_alpha;
    float angle;

    float fade_timer;
} RayData;

static float angles_array[BEAM_COUNT];
static RayData ray_data[BEAM_COUNT];

static bool started = false;
static bool fading = false;

static int current_ray = 0;
static float timer = 0;

static void prepare_angles_array() {
    // Fill array
    for (int i = 0; i < BEAM_COUNT; i++) {
        angles_array[i] = normalize_angle(ANGLE_START + i * ANGLE_STEP);
    }

    // Swap around
    for (int i = BEAM_COUNT - 1; i > 0; i--) {
        int j = random_int(0, i);
        float temp = angles_array[i];
        angles_array[i] = angles_array[j];
        angles_array[j] = temp;
    }
}

static void prepare_rays() {
    for (int i = 0; i < BEAM_COUNT; i++) {
        ray_data[i].position.x = level_info.wall_x;
        ray_data[i].position.y = level_info.wall_y;

        ray_data[i].start_delay = i * STAGGER_STEP + STAGGER_BASE + STAGGER_RAND * random_float(-1, 1);
        ray_data[i].timer = 0;

        ray_data[i].target_h = W_MID + W_JITTER * random_float(-1, 1);
        ray_data[i].tween_duration = DUR_BASE + DUR_JITTER * random_float(-1, 1);
        ray_data[i].start_alpha = CLAMP(ALPHA_BASE + ALPHA_JITTER * random_float(-1, 1), 0, 1);
        ray_data[i].angle = normalize_angle(angles_array[i] + ANGLE_STEP * randomf() + 90);

        ray_data[i].fade_timer = 0;
    }
}

static void draw_ray(float x, float y, float length, float startWidth, float endWidth, float angle, float t, u32 color) {
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

        verts[i].x = verts[i].x - state.camera_x;

        verts[i].y = SCREEN_HEIGHT - (verts[i].y - state.camera_y);

        verts[i].x = get_mirror_x(verts[i].x, state.mirror_factor);
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

void rays_start() {
    started = true;
    fading = false;
    current_ray = 0;
    timer = 0;
    prepare_angles_array();
    prepare_rays();
}

void rays_start_fade() {
    fading = true;
    
    for (int i = 0; i < BEAM_COUNT; i++) {
        ray_data[i].fade_timer = 0;
    }
}

void draw_rays(float delta) {
    if (!started) return;

    // The rays tick as they draw, so don't let extra eyes speed them up
    if (is_extra_eye()) delta = 0;

    for (int i = 0; i < BEAM_COUNT; i++) {
        RayData *data = &ray_data[i];

        // Check if it should start
        if (data->start_delay < timer) {
            float x = data->position.x + 30;
            float y = data->position.y;
            
            float startWidth = data->target_h / 4;
            float endWidth = data->target_h;

            const float length = MAX_BEAM_W;

            float elapsed = data->timer;
            float duration = data->tween_duration;

            float alpha = data->start_alpha * 255;

            if (fading) {
                alpha = easeValue(EASE_LINEAR, alpha, 0, data->fade_timer, RAY_FADE_DURATION, 1);
                data->fade_timer += delta;
            }

            Color col = get_p2_if_black(p1_color);

            u32 color = C2D_Color32(col.r, col.g, col.b, alpha);

            float t = easeValue(QUAD_OUT, 0, 1, elapsed, duration, 1);

            draw_ray(x, y, length, startWidth, endWidth, data->angle, t, color);

            data->timer += delta;
        }
    }

    timer += delta;

    // Remove rays
    if (ray_data[0].fade_timer > RAY_FADE_DURATION) {
        started = false;
        fading = false;
    }
}