#include "graphics.h"
#include "objects.h"
#include "main.h"
#include "math_helpers.h"
#include "color_channels.h"
#include <stdlib.h>
#include "mp3_player.h"
#include "icons.h"
#include "menus/icon_kit.h"
#include "menus/palette_kit.h"

#include "player/player.h"
#include "state.h"
#include "player/collision.h"

#include "utils/gfx.h"

#include "particles/object_particles.h"
#include "particles/circles.h"

#include "menus/settings.h"
#include "menus/gameplay.h"

const Color white = { 255, 255, 255 };

int sprite_count = 0;

C2D_SpriteSheet spriteSheet;
C2D_SpriteSheet spriteSheet2;
C2D_SpriteSheet glowSheet;
C2D_SpriteSheet bgSheet;
C2D_SpriteSheet bg2Sheet;
C2D_SpriteSheet groundSheet;
C2D_SpriteSheet iconSheet;
C2D_SpriteSheet trailSheet;

static SortItem buf_a[MAX_SPRITES];
static SortItem buf_b[MAX_SPRITES];

static SpriteObject viewable_objects[MAX_SPRITES];
static SpriteObject *viewable_objects_ptr[MAX_SPRITES];

bool p1_trail = false;
int current_fading_effect = FADE_NONE;
int current_pulserod_ball_image = 0;

SpriteTemplate sprite_templates[GAME_OBJECT_COUNT]; // global cache

#define LUT_SIZE 256

uint8_t opacityLUT[LUT_SIZE];

void make_opacity_lut() {
    for (int i = 0; i < LUT_SIZE; i++) {
        float x = (float)i / (LUT_SIZE - 1);  // normalize to [0,1]

        float y = 0.175656971639325f * powf(7.06033051530761f, x)
                - 0.213355914301931f;

        // clamp
        if (y < 0.0f) y = 0.0f;
        if (y > 1.0f) y = 1.0f;

        opacityLUT[i] = (uint8_t)(y * 255.0f + 0.5f);
    }
}

float get_opacity(float opacity) {
    int index = (int)(opacity * 255.0f + 0.5f);
    index = index < 0 ? 0 : (index > 255 ? 255 : index);

    uint8_t result = opacityLUT[index];
    return result / 255.f;
}

static C2D_SpriteSheet *get_sprite_sheet(int index, int *rel_index) {
    if (index < SPRITESHEET2_START) {
        *rel_index = index;
        return &spriteSheet;
    }

    *rel_index = index - SPRITESHEET2_START;
    return &spriteSheet2;
}

Color get_color_abgr8(u32 color) {
    Color col;
    col.r = R_ABGR8(color);
    col.g = G_ABGR8(color);
    col.b = B_ABGR8(color);

    return col;
}

void update_player_colors() {
    Color p1 = get_color_abgr8(colors[selected_p1]);
    Color p2 = get_color_abgr8(colors[selected_p2]);
    Color glow = get_color_abgr8(colors[selected_glow]);


    set_player_colors(p1, p2, glow);
}

Color get_white_if_black(Color color) {
    if (color.r == 0 && color.g == 0 && color.b == 0) return white;
    
    return color;
}

void set_player_colors(Color p1, Color p2, Color glow) {
    p1_color = p1;
    p2_color = p2;
    glow_color = glow;
}

void cache_all_sprites() {
    for (int id = 0; id < GAME_OBJECT_COUNT; id++) {
        const GameObject* obj = &game_objects[id];

        // Skip if object has no texture
        if (obj->texture < 0) continue;

        int tex;
        C2D_SpriteSheet *sheet = get_sprite_sheet(obj->texture, &tex);

        C2D_SpriteFromSheet(&sprite_templates[id].parent_template, *sheet, tex);
        C3D_TexSetFilter(sprite_templates[id].parent_template.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetCenter(&sprite_templates[id].parent_template, 0.5f, 0.5f);

        // Get glow frame
        if (obj->glow_frame >= 0) {
            C2D_SpriteFromSheet(&sprite_templates[id].glow_template, glowSheet, obj->glow_frame);
            C3D_TexSetFilter(sprite_templates[id].glow_template.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&sprite_templates[id].glow_template, 0.5f, 0.5f);
        }

        // Children
        sprite_templates[id].child_count = obj->child_count;
        if (obj->child_count > 0) {
            sprite_templates[id].child_templates = malloc(sizeof(C2D_Sprite) * obj->child_count);
            for (int i = 0; i < obj->child_count; i++) {
                const ChildSprite* c = &obj->children[i];
                if (c->texture < 0) continue;

                int c_tex;
                C2D_SpriteSheet *c_sheet = get_sprite_sheet(c->texture, &c_tex);

                C2D_SpriteFromSheet(&sprite_templates[id].child_templates[i], *c_sheet, c_tex);
                C3D_TexSetFilter(sprite_templates[id].child_templates[i].image.tex, GPU_LINEAR, GPU_LINEAR);
                C2D_SpriteSetCenter(&sprite_templates[id].child_templates[i], 0.5f, 0.5f);
            }
        } else {
            sprite_templates[id].child_templates = NULL;
        }
    }
}

void free_cached_sprites() {
    for (int i = 0; i < GAME_OBJECT_COUNT; i++) {
        if (sprite_templates[i].child_templates)
            free(sprite_templates[i].child_templates);
    }
}

float mirror_angle(float angle, bool hflip, bool vflip)
{
    if (hflip && vflip) {
        angle += 180.0f;
    } else if (hflip) {
        angle = 180.0f - angle;
    } else if (vflip) {
        angle = -angle;
    }

    return normalize_angle(angle);
}

inline int get_color_channel(int col_type, int obj, const GameObject *game_obj) {
    int col_channel = game_obj->base_color;
    if (col_type == COLOR_TYPE_BLACK) col_channel = 0;
    else if (col_type == COLOR_TYPE_WHITE) col_channel = -1;
    else {
        if (objects.v1p9_col_channel[obj]) {
            if (col_type == COLOR_TYPE_DETAIL) col_channel = objects.v1p9_col_channel[obj];
        } else {
            if (objects.col_channel[obj]) {
                if (col_type == COLOR_TYPE_BASE) {
                    col_channel = objects.col_channel[obj];
                } else if (!obj_has_main(game_obj)) {
                    col_channel = objects.col_channel[obj];
                }
            }

            if (objects.detail_col_channel[obj]) {
                if (col_type == COLOR_TYPE_DETAIL) {
                    if (obj_has_main(game_obj)) {
                        col_channel = objects.detail_col_channel[obj];
                    }
                }
            }
        }
    }
    return col_channel;
}

float get_fading_obj_fade(float x, float right_edge) {

    if (!state.dead) {
        float fading_obj_width = FADING_OBJ_WIDTH;
        if (x < FADING_OBJ_PADDING || x > right_edge - FADING_OBJ_PADDING)
            return 1.f;
        else if (x < FADING_OBJ_PADDING + fading_obj_width)
            return clampf(1.f - ((x - FADING_OBJ_PADDING) / fading_obj_width), 0.05f, 1.f);
        else if (x > right_edge - fading_obj_width - FADING_OBJ_PADDING)
            return clampf(1.f - ((right_edge - (x + FADING_OBJ_PADDING)) / fading_obj_width), 0.05f, 1.f);
        else
            return 0.05f;
    }

    return 1.f;
}

int get_glow_channel(int id) {
    switch (id) {
        case 143:
        case 177:
        case 178:
        case 179:
        case 183:
        case 184:
        case 185:
        case 186:
        case 187:
        case 188:
            return CHANNEL_LBG_NOLERP;
        case 144:
        case 145:
        case 146:
        case 147:
        case 204:
        case 205:
        case 206:
        case 459:
        case 673:
        case 674:
        case 740:
        case 741:
        case 742:
            return CHANNEL_LBG;
    }
    return CHANNEL_OBJ_BLENDING;
}

const int obj_9_random_layers[4] = {
    585,
    589,
    590,
    589
};

const int obj_135_random_layers[4] = {
    610,
    611,
    612,
    613
};

int get_obj_random_layer(int obj, int id) {
    switch (id) {
        case 9:
            return obj_9_random_layers[objects.random[obj] & 0b11];
        case 135:
            return obj_135_random_layers[objects.random[obj] & 0b11];
    }
    return -1;
}

float get_rotation_speed(int id) {
    switch (id) {
        case 88: 
        case 89:
        case 98:
        case 183:
        case 184:
        case 185:
        case 186:
        case 187:
        case 188:
        case 397:
        case 398:
        case 399:
        case 675:
        case 676:
        case 677:
        case 678:
        case 679:
        case 680:
        case 740:
        case 741:
        case 742:
            return 360.f;
        
        case 85:
        case 86:
        case 87:
        case 97:
        case 137:
        case 138:
        case 139:
        case 154:
        case 155:
        case 156:
        case 180:
        case 181:
        case 182:
        case 222:
        case 223:
        case 224:
        case 375:
        case 376:
        case 377:
        case 378:
        case 394:
        case 395:
        case 396:
            return 180.f;
    }
    return 0.f;
}


float get_object_pulse(float amplitude, int id, int layer) {
    switch (id) {
        case 36:
        case 84:
        case 141:
            return map_range(amplitude, 0.f, 1.f, 0.3f, 1.2f);
        case 15:
        case 16:
        case 17:
            if (layer == 2) {    
                return amplitude;
            }
            return 1.0f;
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 60:
        case 148:
        case 149:
        case 405:
            return amplitude;
        case 132:
        case 133:
        case 136:
        case 150:
        case 236:
        case 460:
        case 494:
        case 495:
        case 496:
        case 497:
            return map_range(amplitude, 0.f, 1.f, 0.6f, 1.2f);
    }
    return 1.0f;
}

void spawn_object_at(
    int obj_game,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale
) {
    const GameObject* obj = &game_objects[id];

    float rad = C3D_AngleFromDegrees(adjust_angle(deg, 0, state.mirror_mult < 0));
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    int flip_x_mult = (flip_x ? -1 : 1);
    int flip_y_mult = (flip_y ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = sin_r;
    float m11 = -cos_r;

    float sx = scale * flip_x_mult;
    float sy = scale * flip_y_mult;

    if (sprite_count >= MAX_SPRITES - 1) return;

    // Skip if no texture
    if (obj->texture >= 0) {
        SpriteObject *vo = &viewable_objects[sprite_count];

        float local_x = obj->x * flip_x_mult;
        float local_y = obj->y * flip_y_mult;

        float rot_x = local_x * m00 + local_y * m01;
        float rot_y = local_x * m10 + local_y * m11;

        float p_x = x + rot_x * scale;
        float p_y = y + rot_y * scale;

        int random_layer = get_obj_random_layer(obj_game, id);
        if (random_layer < 0) {
            vo->spr = sprite_templates[id].parent_template;
        } else {
            C2D_Sprite rnd = { 0 };
            vo->spr = rnd;
            C2D_SpriteFromSheet(&vo->spr, spriteSheet, random_layer);
            C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);
        }

        float pulse_scale = get_object_pulse(amplitude, id, 0);

        C2D_SpriteSetPos(&vo->spr, p_x, p_y);
        C2D_SpriteSetScale(&vo->spr, sx * pulse_scale, sy * pulse_scale);
        C2D_SpriteSetRotation(&vo->spr, rad);

        vo->obj = obj_game;
        vo->layer = 0;
        vo->col_type = obj->color_type;
        vo->opacity = obj->opacity;
        vo->col_channel = get_color_channel(obj->color_type, obj_game, obj);
        viewable_objects_ptr[sprite_count] = vo;
        sprite_count++;
    }

    // Skip if no glow frame
    if (glowEnabled && obj->glow_frame >= 0) {
        if (sprite_count >= MAX_SPRITES - 1) return;

        SpriteObject *vo = &viewable_objects[sprite_count];

        vo->spr = sprite_templates[id].glow_template;

        float pulse_scale = get_object_pulse(amplitude, id, 1);

        C2D_SpriteSetPos(&vo->spr, x, y);
        C2D_SpriteSetScale(&vo->spr, sx * pulse_scale, sy * pulse_scale);
        C2D_SpriteSetRotation(&vo->spr, rad);

        vo->obj = obj_game;
        vo->layer = 1;
        vo->col_type = COLOR_TYPE_BASE;
        vo->opacity = 0.5f;
        vo->col_channel = get_glow_channel(id);
        viewable_objects_ptr[sprite_count] = vo;
        sprite_count++;
    }

    // Render children
    for (int i = 0; i < obj->child_count; i++) {
        const ChildSprite* c = &obj->children[i];
        
        if (sprite_count >= MAX_SPRITES - 1) return;
        
        // Skip if no texture
        if (c->texture >= 0) {    
            SpriteObject *vo = &viewable_objects[sprite_count];

            float c_local_x = c->x * flip_x_mult;
            float c_local_y = c->y * flip_y_mult;

            float c_rot_x = c_local_x * m00 + c_local_y * m01;
            float c_rot_y = c_local_x * m10 + c_local_y * m11;

            float c_x = x + c_rot_x * scale;
            float c_y = y + c_rot_y * scale;

            int c_flip_x_mult = (c->flip_x ? -1 : 1);
            int c_flip_y_mult = (c->flip_y ? -1 : 1);

            vo->spr = sprite_templates[id].child_templates[i]; 

            float pulse_scale = get_object_pulse(amplitude, id, i + 2);

            C2D_SpriteSetPos(&vo->spr, c_x, c_y);
            if (id < 15 || id > 17) {
                C2D_SpriteSetScale(&vo->spr, c->scale_x * c_flip_x_mult * sx * pulse_scale,
                                          c->scale_y * c_flip_y_mult * sy * pulse_scale);
                C2D_SpriteSetRotation(&vo->spr, C3D_AngleFromDegrees(c->rot) + rad);
            } else {
                C2D_SpriteSetScale(&vo->spr, fabsf(c->scale_x * c_flip_x_mult * sx * pulse_scale),
                                          fabsf(c->scale_y * c_flip_y_mult * sy * pulse_scale));
            }

            vo->obj = obj_game;
            vo->layer = i + 2;
            vo->col_type = c->color_type;
            vo->opacity = c->opacity;
            vo->col_channel = get_color_channel(c->color_type, obj_game, obj);
            viewable_objects_ptr[sprite_count] = vo;
            sprite_count++;
        }
    }
}

static inline uint32_t make_sort_key(SpriteObject *s)
{
    const int obj = s->obj;

    if (obj == -1) {
        return ((5 + 8) << 17) | (0 << 16) | (0 << 8) | 0;
    }

    const int id = objects.id[obj];
    const GameObject *game_obj = &game_objects[id];

    int zlayer = objects.zlayer[obj] ? objects.zlayer[obj] : game_obj->z_layer;

    // Blending makes zlayer one 
    int col_channel = objects.col_channel[obj];

    if (s->layer == 1) {
        zlayer--;
    } else if (col_channel > 0 && (channels[col_channel].blending ^ ((zlayer & 1) == 0))) {
        zlayer--;
    }

    int child_z = 0;
    int tex = game_obj->texture;

    if (s->layer > 1) {
        const ChildSprite *child = &game_obj->children[s->layer - 2];
        child_z = child->z - 1;
        tex = child->texture;
        zlayer += child->z_layer_offset;
    }
    
    int sheet;
    if (s->layer == 1) {
        sheet = 2;
    } else {
        sheet = tex < SPRITESHEET2_START ? 1 : 0;
    }
    
    int zorder = objects.zorder[obj] ? objects.zorder[obj] : game_obj->z_order;

    // Move the pulserod ball
    if (id >= 15 && id <= 17 && s->layer == 2) {
        zlayer += 2;
    } 

    s->zlayer = zlayer;

    uint32_t zl = (uint32_t)(zlayer + 8);     // fits in 7 bits
    uint32_t zs = (uint32_t)(sheet);          // fits in 1 bit
    uint32_t zo = (uint32_t)(zorder + 128);   // fits in 8 bits
    uint32_t cz = (uint32_t)(child_z + 128);  // fits in 8 bits

    return (zl << 17) | (zs << 16) | (zo << 8) | cz;
}

#define VIEW_OBJECTS (12 * 6)
#define INSERTION_SORT_THRESHOLD 16

void sort_viewable_objects(SpriteObject **objects, int count)
{
    if (count <= 1) return;

    for (int i = 0; i < count; i++) {
        buf_a[i].obj = objects[i];
        buf_a[i].key = make_sort_key(objects[i]);
    }

    SortItem *src = buf_a;
    SortItem *dst = buf_b;

    for (int pass = 0; pass < 3; pass++) {
        uint16_t buckets[256] = {0};
        int shift = pass * 8;

        for (int i = 0; i < count; i++) {
            buckets[(src[i].key >> shift) & 0xFF]++;
        }

        uint16_t sum = 0;
        for (int i = 0; i < 256; i++) {
            uint16_t t = buckets[i];
            buckets[i] = sum;
            sum += t;
        }

        for (int i = 0; i < count; i++) {
            uint8_t b = (src[i].key >> shift) & 0xFF;
            dst[buckets[b]++] = src[i];
        }

        SortItem *tmp = src;
        src = dst;
        dst = tmp;
    }

    for (int i = 0; i < count; i++) {
        objects[i] = src[i].obj;
    }
}

int get_object_layers(int id) {
    int count = 0;
    if (id < 0 || id >= GAME_OBJECT_COUNT) return 0;

    const GameObject obj = game_objects[id];
    if (obj.texture >= 0) count++;
    
    for (size_t c = 0; c < obj.child_count; c++) {
        if (obj.children[c].texture >= 0) count++;
    }
    return count;
}

bool object_fades(int obj) {
    switch (objects.id[obj]) {
        case 144:
        case 145:
        case 146:
        case 147:
        case 204:
        case 205:
        case 206:
        case 459:
        case 673:
        case 674:
        case 740:
        case 741:
        case 742:
            return true;
    }
    return false;
}

int obj_edge_fade(float x, int right_edge) {
    if (x < 0 || x > right_edge)
        return 0;
    else if (x < FADE_WIDTH)
        return (int)(255.0f * (x / FADE_WIDTH));
    else if (x > right_edge - FADE_WIDTH)
        return (int)(255.0f * ((right_edge - x) / FADE_WIDTH));
    else
        return 255;
}

int get_xy_fade_offset(float x, int right_edge) {
    int fade = obj_edge_fade(x, right_edge);
    return (255 - fade) / 2;
}

float get_in_scale_fade(float x, int right_edge) {
    int fade = obj_edge_fade(x, right_edge);
    return (fade / 255.f);
}

float get_out_scale_fade(float x, int right_edge) {
    int fade = 255 - obj_edge_fade(x, right_edge);
    return 1 + ((fade / 255.f) / 2);
}

int get_obj_opacity(int obj, float x) {
    int opacity = obj_edge_fade(x, SCREEN_WIDTH / SCALE);

    switch (objects.id[obj]) {
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 309:
        case 311:
        case 687:
        case 688:
            if (objects.transition_applied[obj] == FADE_NONE) opacity = 255;
            break;
            
        case 207:
        case 208:
        case 209:
        case 210:
        case 211:
        case 212:
        case 213:
        case 693:
        case 694:
        case 331:
        case 333:
            bool blending = channels[objects.detail_col_channel[obj]].blending;
            if (!blending && objects.transition_applied[obj] == FADE_NONE) opacity = 255;
            break;
    }

    return opacity;
}


void handle_special_fading(int obj, float calc_x, float calc_y) {
    switch (current_fading_effect) {
        case FADE_INWARDS:
            if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                objects.transition_applied[obj] = FADE_UP;
            } else {
                objects.transition_applied[obj] = FADE_DOWN;
            }
            break;
        case FADE_OUTWARDS:
            if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                objects.transition_applied[obj] = FADE_DOWN;
            } else {
                objects.transition_applied[obj] = FADE_UP;
            }
            break;
        case FADE_CIRCLE_LEFT:
            if (calc_x > (SCREEN_WIDTH / SCALE / 2)) {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transition_applied[obj] = FADE_UP_STATIONARY;
                } else {
                    objects.transition_applied[obj] = FADE_DOWN_STATIONARY;
                }
            } else {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transition_applied[obj] = FADE_UP_SLOW;
                } else {
                    objects.transition_applied[obj] = FADE_DOWN_SLOW;
                }
            }
            break;
        case FADE_CIRCLE_RIGHT:
            if (calc_x > (SCREEN_WIDTH / SCALE / 2)) {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transition_applied[obj] = FADE_UP_SLOW;
                } else {
                    objects.transition_applied[obj] = FADE_DOWN_SLOW;
                }
            } else {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transition_applied[obj] = FADE_UP_STATIONARY;
                } else {
                    objects.transition_applied[obj] = FADE_DOWN_STATIONARY;
                }
            }
            break;
        default:
            objects.transition_applied[obj] = current_fading_effect;  
    }   
}

void get_fade_vars(int obj, float x, int *fade_x, int *fade_y, float *fade_scale) {
    switch (objects.transition_applied[obj]) {
        case FADE_NONE:
            break;
        case FADE_UP:
            *fade_y = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE);
            break;
        case FADE_DOWN:
            *fade_y = -get_xy_fade_offset(x, SCREEN_WIDTH / SCALE);
            break;
        case FADE_RIGHT:
            *fade_x = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE);
            break;
        case FADE_LEFT:
            *fade_x = -get_xy_fade_offset(x, SCREEN_WIDTH / SCALE);
            break;
        case FADE_SCALE_IN:
            *fade_scale = get_in_scale_fade(x, SCREEN_WIDTH / SCALE);
            break;
        case FADE_SCALE_OUT:
            *fade_scale = get_out_scale_fade(x, SCREEN_WIDTH / SCALE);
            break;
        case FADE_UP_SLOW:
            *fade_x = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) * ((current_fading_effect == FADE_CIRCLE_RIGHT) ? 1 : -1);
        case FADE_UP_STATIONARY:
            *fade_y = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) / 3;
            break;
        case FADE_DOWN_SLOW:
            *fade_x = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) * ((current_fading_effect == FADE_CIRCLE_RIGHT) ? 1 : -1);
        case FADE_DOWN_STATIONARY:
            *fade_y = -get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) / 3;
            break;
    }
}

void change_blending(bool blending) {
    if (blending) {
        C2D_Flush();
        C3D_AlphaBlend(
            GPU_BLEND_ADD, GPU_BLEND_ADD,
            GPU_SRC_ALPHA, GPU_ONE,
            GPU_ONE, GPU_ZERO
        );
        C2D_Prepare();
    } else {
        C2D_Flush();
        C3D_AlphaBlend(
            GPU_BLEND_ADD, GPU_BLEND_ADD, 
            GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, 
            GPU_ONE, GPU_ZERO);
        C2D_Prepare();
    }
}

void draw_background(float x, float y) {
    C2D_ImageTint tint = { 0 };
    Color col = channels[CHANNEL_BG].color;
    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    float offset = 512 * BACKGROUND_SCALE;

    float calc_x = positive_fmodf(x, offset);
    float draw_y = -y;

    int bg_id = level_info.background_id;

    for (int i = 0; i < 2; i++) {
        C2D_Sprite bg = { 0 };
        // Calculate position for each tile
        float draw_x = -calc_x + i * offset;

        
        C2D_SpriteFromSheet(&bg, bg_id < 4 ? bgSheet : bg2Sheet, bg_id & 0b11);
        C3D_TexSetFilter(bg.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetPos(&bg, (int)draw_x, (int)draw_y);
        C2D_SpriteSetScale(&bg, BACKGROUND_SCALE, BACKGROUND_SCALE);
        C2D_DrawSpriteTinted(&bg, &tint);
    }
}

void draw_ground(float cam_x, float cam_y, float y, bool is_ceiling, int screen_width) {
    change_blending(false);
    int mult = (is_ceiling ? -1 : 1);
    
    C2D_ImageTint tint = { 0 };
    Color col = channels[CHANNEL_GROUND].color;
    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    if (is_ceiling) y += GROUND_SIZE;

    // First draw the ground
    float calc_x = 0 - positive_fmodf(cam_x, GROUND_SIZE);
    float calc_y = SCREEN_HEIGHT - ((y - cam_y));

    for (float i = -GROUND_SIZE; i < (screen_width / SCALE) + GROUND_SIZE; i += GROUND_SIZE) {
        C2D_Sprite ground = { 0 };
        C2D_SpriteFromSheet(&ground, groundSheet, level_info.ground_id + 1);
        C3D_TexSetFilter(ground.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetPos(&ground, calc_x + i, calc_y);
        C2D_SpriteSetScale(&ground, 1.f, mult);
        C2D_DrawSpriteTinted(&ground, &tint);
    }

    // Then draw the line
    if (channels[CHANNEL_LINE].blending) {
        change_blending(true);
    }

    col = channels[CHANNEL_LINE].color;
    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    float line_offset = -((GROUND_SIZE / 2) - (LINE_HEIGHT / 2)) * mult;
    C2D_Sprite line = { 0 };
    C2D_SpriteFromSheet(&line, groundSheet, 0);
    C3D_TexSetFilter(line.image.tex, GPU_LINEAR, GPU_LINEAR);
    C2D_SpriteSetPos(&line, screen_width / SCALE / 2, (int)((GROUND_SIZE / 2) + calc_y + line_offset));
    C2D_SpriteSetCenter(&line, 0.5f, 0.5f);
    C2D_DrawSpriteTinted(&line, &tint);

    if (channels[CHANNEL_LINE].blending) {
        change_blending(false);
    }
}

float object_creating_time = 0;
float object_sorting_time = 0;
float object_drawing_time = 0;


void create_objects() {
    sprite_count = 0;

    // Player sprite
    SpriteObject *vo = &viewable_objects[sprite_count];

    C2D_Sprite spr = { 0 };
    vo->spr = spr;
    vo->obj = -1;
    vo->layer = 0;
    vo->col_type = 0;
    vo->opacity = 1.f;
    vo->col_channel = 0;
    viewable_objects_ptr[sprite_count] = vo;
    sprite_count++;

    int width = ceilf((SCREEN_WIDTH_AREA) / SECTION_SIZE);
    int height = ceilf((SCREEN_HEIGHT_AREA) / SECTION_SIZE);
    int cam_sx = (int)((state.camera_x) / SECTION_SIZE);
    int cam_sy = (int)((state.camera_y - CAMERA_Y_OFFSET) / SECTION_SIZE);
    u64 start = svcGetSystemTick();
    // Create sprites
    for (int x = -1; x <= width; x++) {
        for (int y = -1; y <= height; y++) {
            int sx = cam_sx + x;
            int sy = cam_sy + y;
            if (sx < 0) continue;
            if (sy < 0) continue;

            Section *sec = get_or_create_section(sx, sy);
            for (int i = 0; i < sec->object_count; i++) {
                int obj = sec->objects[i];
                
                float calc_x = (objects.x[obj] - state.camera_x);
                float calc_y = SCREEN_HEIGHT - ((objects.y[obj] - state.camera_y));  
                if (calc_x < -60 || calc_x >= (SCREEN_WIDTH / SCALE) + 60) continue;
                if (calc_y < -60 || calc_y >= (SCREEN_HEIGHT / SCALE) + 60) continue;

                // Skip invalid objects
                if (objects.id[obj] > 745) {
                    continue;
                }

                int fade_val = obj_edge_fade(calc_x, SCREEN_WIDTH / SCALE);
                bool fade_edge = (fade_val == 255 || fade_val == 0);

                if (fade_edge) handle_special_fading(obj, calc_x, calc_y);
                int fade_x = 0;
                int fade_y = 0;

                float fade_scale = 1.f;

                get_fade_vars(obj, calc_x, &fade_x, &fade_y, &fade_scale);

                objects.rotation[obj] += (((objects.random[obj] & 1) ? -get_rotation_speed(objects.id[obj]) : get_rotation_speed(objects.id[obj]))) * DT;
                
                // Handle special fade types
                if (objects.transition_applied[obj] == FADE_DOWN_STATIONARY || objects.transition_applied[obj] == FADE_UP_STATIONARY) {
                    if (fade_val < 255) {
                        if (calc_x > (SCREEN_WIDTH / SCALE) / 2) {
                            calc_x = SCREEN_WIDTH / SCALE - FADE_WIDTH;
                        } else {
                            calc_x = FADE_WIDTH;
                        }
                    }
                }

                spawn_object_at(
                    obj,
                    objects.id[obj],
                    get_mirror_x(calc_x, state.mirror_factor) + fade_x,
                    calc_y + fade_y,
                    objects.rotation[obj],
                    objects.flippedH[obj] ^ (state.mirror_mult < 0),
                    objects.flippedV[obj],
                    fade_scale
                );

                spawn_object_particles(obj);
            }
        }
    }
    
    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    object_creating_time = ticks / CPU_TICKS_PER_MSEC;
    
    start = svcGetSystemTick();
    // Sort
    sort_viewable_objects(viewable_objects_ptr, sprite_count);
    end = svcGetSystemTick();
    ticks = end - start;
    object_sorting_time = ticks / CPU_TICKS_PER_MSEC;

    for (size_t s = 0; s < sprite_count; s++) {
        SpriteObject *obj = viewable_objects_ptr[s];
        if (obj->obj != -1) {
            int col_channel = obj->col_channel;

            ColorChannel col;

            if (col_channel < 0) {
                col.color.r = 255;
                col.color.g = 255;
                col.color.b = 255;
                col.blending = false;
            } else {
                col = channels[col_channel];
            }
            
            int game_object = obj->obj;
            float x = ((objects.x[game_object] - state.camera_x));
            
            float opacity = obj->opacity;
            if (object_fades(game_object)) {
                opacity *= get_fading_obj_fade(x, SCREEN_WIDTH / SCALE);
            }
            
            C2D_PlainImageTint(&obj->tint, C2D_Color32(col.color.r, col.color.g, col.color.b, get_obj_opacity(game_object, x) * opacity), 1.f);
        }
    }
}

void draw_objects() {
    u64 start = svcGetSystemTick();
    int blend_enabled = false;
    // Draw
    for (size_t s = 0; s < sprite_count; s++) {
        SpriteObject *obj = viewable_objects_ptr[s];

        if (obj->obj != -1) {
            int col_channel = obj->col_channel;
            
            ColorChannel col;

            if (col_channel < 0) {
                col.color.r = 255;
                col.color.g = 255;
                col.color.b = 255;
                col.blending = false;
            } else {
                col = channels[col_channel];
            }

            if (col.blending && !blend_enabled) {
                change_blending(true);
                blend_enabled = true;
            } else if (!col.blending && blend_enabled) {
                change_blending(false);
                blend_enabled = false;
            }

            // Cull invisible objects
            if ((col.color.r | col.color.g | col.color.b) == 0 && blend_enabled) continue;
            
            C2D_DrawSpriteTinted(&obj->spr, &obj->tint);
/*
            float calc_x = ((objects.x[game_object] - state.camera_x));
            float calc_y = SCREEN_HEIGHT - ((objects.y[game_object] - state.camera_y));  
            float width = objects.width[game_object];
            float height = objects.height[game_object];

            C2D_DrawRectSolid(calc_x - width/2, calc_y - height/2, 0, width, height, 
                C2D_Color32(0, 0, 255, 255));
*/
        } else {   
            change_blending(true);
            draw_use_effects(GFX_TOP);
            draw_object_particles();
            for (int i = 0; i < 2; i++) {
                drawParticleSystem(&drag_particles[i], 0, 0, 1.f);
                drawParticleSystem(&ship_fire_particles[i], 0, 0, 1.f);
                drawParticleSystem(&ship_secondary_particles[i], 0, 0, 1.f);
                drawParticleSystem(&secondary_particles[i], 0, 0, 1.f);
                drawParticleSystem(&burst_particles[i], 0, 0, 1.f);
                drawParticleSystem(&land_particles[i], 0, 0, 1.f);
                drawParticleSystem(&explosion_particles[i], 0, 0, 1.f);
            }
            drawParticleSystem(&brick_destroy_particles, 0, 0, 1.f);
            drawParticleSystem(&glitter_particles, 0, 0, 1.f);
            change_blending(false);
            blend_enabled = false;
            state.current_player = 0;
            
            trail = &trail_p1;
            wave_trail = &wave_trail_p1;

            draw_player(&state.player);
            
            if (state.dual) {
                state.current_player = 1;
                trail = &trail_p2;
                wave_trail = &wave_trail_p2;
                draw_player(&state.player2);
            }  
                    
            change_blending(true);
            for (int i = 0; i < 2; i++) {
                drawParticleSystem(&drag_particles_2[i], 0, 0, 1.f);
            }
            change_blending(false);
        }
    }
    if (!blend_enabled) change_blending(true);
        drawParticleSystem(&slow_speed_particles, 0, 0, 1.f);
        drawParticleSystem(&normal_speed_particles, 0, 0, 1.f);
        drawParticleSystem(&fast_speed_particles, 0, 0, 1.f);
        drawParticleSystem(&faster_speed_particles, 0, 0, 1.f);
        change_blending(false);

    if (!blend_enabled) change_blending(true);
    drawParticleSystem(&slow_speed_particles, 0, 0, 1.f);
    change_blending(false);

    if (state.hitbox_display) {
        for (size_t s = 0; s < sprite_count; s++) {
            SpriteObject *obj = viewable_objects_ptr[s];
            if (obj->obj != -1)     
                draw_hitbox(obj->obj);
            else {
                draw_player_hitbox(&state.player);
                if (state.hitbox_display == 2) draw_hitbox_trail(0);
                
                if (state.dual) {
                    draw_player_hitbox(&state.player2);
                    if (state.hitbox_display == 2) draw_hitbox_trail(1);
                }
            }
        }
    }

    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    object_drawing_time = ticks / CPU_TICKS_PER_MSEC;
}

void update_touch_effect(float delta) {
    touchPosition pos;
    hidTouchRead(&pos);

    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    touch_drag_particles.emitting = false;

    if ((touchEffectEverywhere || (game_state == STATE_GAME && !game_paused)) && (kHeld & KEY_TOUCH) && !get_fade_status()) {
        // Flipped for particles
        float flipped_y = SCREEN_HEIGHT - pos.py;

        // Use effect
        if (kDown & KEY_TOUCH) {
            UseEffect *effect = add_use_effect(pos.px, pos.py, -1, &tap_effect, GFX_BOTTOM);    
            touch_explosion_particles.emitterX = pos.px;
            touch_explosion_particles.emitterY = flipped_y;
            spawnMultipleParticles(&touch_explosion_particles, 50);
            if (effect) {
                Color p1_not_white = get_white_if_black(p1_color);

                effect->def.colorR = p1_not_white.r / 255.f;
                effect->def.colorG = p1_not_white.g / 255.f;
                effect->def.colorB = p1_not_white.b / 255.f;
            }
        }

        touch_drag_particles.emitterX = pos.px;
        touch_drag_particles.emitterY = flipped_y;
        touch_drag_particles.emitting = true;

        
    }
    update_use_effects(delta, GFX_BOTTOM);
    updateParticleSystem(&touch_explosion_particles, delta);
    updateParticleSystem(&touch_drag_particles, delta);
}

void draw_touch_effect() {
    draw_use_effects(GFX_BOTTOM);
    drawParticleSystem(&touch_drag_particles, 0, 0, 1.f);
    drawParticleSystem(&touch_explosion_particles, 0, 0, 1.f);
}

void spawn_icon_at(
    int gamemode,
    int id,
    bool glow,
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale,
    u32 p1_color,
    u32 p2_color,
    u32 glow_color
) {
    const Icon icon = icons[gamemode][id];
    const IconPart *parts = icon.parts;

    float rad = C3D_AngleFromDegrees(deg);
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    int flip_x_mult = (flip_x ? -1 : 1);
    int flip_y_mult = (flip_y ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = sin_r;
    float m11 = -cos_r;

    float sx = scale * flip_x_mult;
    float sy = scale * flip_y_mult;

    C2D_Sprite spr = { 0 };

    C2D_ImageTint tints[icon.part_count];

    for (size_t i = 0; i < icon.part_count; i++) {
        C2D_PlainImageTint(&tints[i], C2D_Color32(255, 255, 255, 255), 1.0f);
    }

    int count = icon.part_count;

    if (!glow) count--;

    C2D_PlainImageTint(&tints[0], p1_color, 1.0f);
    C2D_PlainImageTint(&tints[1], p2_color, 1.0f);
    C2D_PlainImageTint(&tints[icon.part_count - 1], glow_color, 1.0f);

    for (size_t i = 0; i < count; i++) {
        size_t real_index = i;
        // Swap p1 and p2 layers
        if (i==0) real_index = 1;
        else if (i==1) real_index = 0;

        if (gamemode == GAMEMODE_BIRD) {
            if (i==2) real_index = 0;
            else if (i < 2) real_index++;
        }
        
        const IconPart *part = &parts[real_index];

        if (part->texture >= 0) {

            float local_x = part->x * flip_x_mult;
            float local_y = part->y * flip_y_mult;

            float rot_x = local_x * m00 + local_y * m01;
            float rot_y = local_x * m10 + local_y * m11;

            float p_x = x + rot_x * scale;
            float p_y = y + rot_y * scale;

            C2D_SpriteFromSheet(&spr, iconSheet, part->texture);
            C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
            C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

            C2D_SpriteSetPos(&spr, p_x, p_y);
            C2D_SpriteSetScale(&spr, sx, sy);
            C2D_SpriteSetRotation(&spr, rad);

            C2D_DrawSpriteTinted(&spr, &tints[real_index]);
        }
    }
}

void spawn_p1_layer_at(
    int gamemode,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale,
    u32 p1_color
) {
    const Icon icon = icons[gamemode][id];
    const IconPart *parts = icon.parts;

    float rad = C3D_AngleFromDegrees(deg);
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    int flip_x_mult = (flip_x ? -1 : 1);
    int flip_y_mult = (flip_y ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = sin_r;
    float m11 = -cos_r;

    float sx = scale * flip_x_mult;
    float sy = scale * flip_y_mult;

    C2D_Sprite spr = { 0 };

    C2D_ImageTint tint;

    C2D_PlainImageTint(&tint, p1_color, 1.0f);
        
    const IconPart *part = &parts[0];

    if (part->texture >= 0) {
        float local_x = part->x * flip_x_mult;
        float local_y = part->y * flip_y_mult;

        float rot_x = local_x * m00 + local_y * m01;
        float rot_y = local_x * m10 + local_y * m11;

        float p_x = x + rot_x * scale;
        float p_y = y + rot_y * scale;

        C2D_SpriteFromSheet(&spr, iconSheet, part->texture);
        C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
        C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

        C2D_SpriteSetPos(&spr, p_x, p_y);
        C2D_SpriteSetScale(&spr, sx, sy);
        C2D_SpriteSetRotation(&spr, rad);

        C2D_DrawSpriteTinted(&spr, &tint);
    }
}

void spawn_glow_layer_at(
    int gamemode,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale,
    u32 glow_color
) {
    const Icon icon = icons[gamemode][id];
    const IconPart *parts = icon.parts;

    float rad = C3D_AngleFromDegrees(deg);
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    int flip_x_mult = (flip_x ? -1 : 1);
    int flip_y_mult = (flip_y ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = sin_r;
    float m11 = -cos_r;

    float sx = scale * flip_x_mult;
    float sy = scale * flip_y_mult;

    C2D_Sprite spr = { 0 };

    C2D_ImageTint tint;

    C2D_PlainImageTint(&tint, glow_color, 1.0f);
        
    const IconPart *part = &parts[icon.part_count - 1];

    if (part->texture >= 0) {
        float local_x = part->x * flip_x_mult;
        float local_y = part->y * flip_y_mult;

        float rot_x = local_x * m00 + local_y * m01;
        float rot_y = local_x * m10 + local_y * m11;

        float p_x = x + rot_x * scale;
        float p_y = y + rot_y * scale;

        C2D_SpriteFromSheet(&spr, iconSheet, part->texture);
        C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
        C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

        C2D_SpriteSetPos(&spr, p_x, p_y);
        C2D_SpriteSetScale(&spr, sx, sy);
        C2D_SpriteSetRotation(&spr, rad);

        C2D_DrawSpriteTinted(&spr, &tint);
    }
}

float approachf(float current, float target, float speed, float smoothing) {
    float diff = target - current;
    float step = diff * smoothing; // smoothing in [0,1], e.g. 0.1 for gentle, 0.5 for fast
    if (fabsf(diff) < speed)
        return target;
    return current + step + (diff > 0 ? speed : -speed);
}


void handle_mirror_transition() {
    state.mirror_factor = approachf(state.mirror_factor, state.intended_mirror_factor, 0.01f, 0.002f);
    state.mirror_speed_factor = approachf(state.mirror_speed_factor, state.intended_mirror_speed_factor, 0.02f, 0.004f);
    if (state.mirror_factor >= 0.5f) {
        state.mirror_mult = -1;
    } else {
        state.mirror_mult = 1;
    }
}