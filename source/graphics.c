#include "graphics.h"
#include "c2d/base.h"
#include "c2d/spritesheet.h"
#include "c3d/maths.h"
#include "level_loading.h"
#include "objects.h"
#include "animations.h"
#include "main.h"
#include "math_helpers.h"
#include "player/robot_anim_data.h"
#include "triggers.h"
#include <stdlib.h>
#include <string.h>
#include "mp3_player.h"
#include "icons.h"
#include "menus/icon_kit.h"
#include "menus/palette_kit.h"

#include "player/player.h"
#include "profiling.h"
#include "state.h"
#include "player/collision.h"

#include "utils/gfx.h"

#include "particles/object_particles.h"
#include "particles/circles.h"
#include "particles/coin_effect.h"
#include "particles/key_effect.h"

#include "menus/settings_hub/settings.h"
#include "menus/gameplay.h"

#include "menus/core/ui_screen.h"

#include "fonts/bigFont.h"
#include "fonts/level_fonts.h"
#include "particles/rays.h"
#include "practice.h"

#include "save/saving.h"
#include "menus/level_select.h"

const Color white = { 255, 255, 255 };

#define KEY_FLOAT_AMP 1.5f
#define KEY_FLOAT_PERIOD 0.8f
#define KEY_FLOAT_BASE 0.0f

static inline float key_float_offset(int obj) {
    float phase = objects.x[obj] * 0.05f;
    return KEY_FLOAT_BASE + KEY_FLOAT_AMP * lut_sin(6.2831853f * (frame_timer / KEY_FLOAT_PERIOD) + phase);
}

static const HSV lighter_hsv = {
    .h = 0.0f,
    .s = 0.65f,
    .v = 1.30f,
    .sChecked = false,
    .vChecked = false
};

static bool color_equal(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

static Color apply_hsv_to_color(Color src, int game_object, bool is_main, int col_channel) {
    bool *valid;
    Color *cached_src, *cached_color;
    HSV *hsv;

    if (is_main) {
        hsv = &objects.main_col_HSV[game_object];
        valid = &objects.cached_main_hsv_valid[game_object];
        cached_src = &objects.cached_main_hsv_src_color[game_object];
        cached_color = &objects.cached_main_hsv_color[game_object];
    } else {
        hsv = &objects.detail_col_HSV[game_object];
        valid = &objects.cached_detail_hsv_valid[game_object];
        cached_src = &objects.cached_detail_hsv_src_color[game_object];
        cached_color = &objects.cached_detail_hsv_color[game_object];
    }

    if (*valid && color_equal(*cached_src, src)) {
        return *cached_color;
    }

    Color out = HSV_combine(src, *hsv);
    *cached_src = src;
    *cached_color = out;
    *valid = true;

    return out;
}

int sprite_count = 0;

static bool blending_state = false;

C2D_SpriteSheet spriteSheet;
C2D_SpriteSheet spriteSheet2;
C2D_SpriteSheet spriteSheet3;
C2D_SpriteSheet animatedSheet;
C2D_SpriteSheet glowSheet;
C2D_SpriteSheet bgSheet;
int loaded_bg_sheet = 0;
C2D_SpriteSheet groundSheet;
C2D_SpriteSheet cube0Sheet;
C2D_SpriteSheet cube1Sheet;
C2D_SpriteSheet shipSheet;
C2D_SpriteSheet ballSheet;
C2D_SpriteSheet ufoSheet;
C2D_SpriteSheet waveSheet;
C2D_SpriteSheet robotSheet;
C2D_SpriteSheet trailSheet;
C2D_SpriteSheet particleSheet;

static SortItem buf_a[MAX_SPRITES];
static SortItem buf_b[MAX_SPRITES];

static SpriteObject *object_sprite_cache;
static SpriteObject *viewable_objects;
static SpriteObject *viewable_objects_ptr[MAX_SPRITES];
static int current_objects[MAX_SPRITES];
static int *object_sprite_start;
static unsigned char *object_sprite_count;
static int current_object_count;
static int render_object_capacity;
static float render_mirror_factor;
static bool render_list_changed;
static SpriteObject player_sprite_cache;

bool p1_trail = false;
float p1_trail_timer = 0;
int current_fading_effect = FADE_SIMPLE;
int current_pulserod_ball_image = 0;

SpriteTemplate sprite_templates[GAME_OBJECT_COUNT]; // global cache

float touch_effect_drag_timer = 0.f;

static C2D_SpriteSheet *get_sprite_sheet(int index, int *rel_index) {
    // Check if index belongs to spritesheet 1 (most objects)
    if (index < SPRITESHEET2_START) {
        *rel_index = index;
        return &spriteSheet;
    }

    if (index < SPRITESHEET3_START) {
        // Return spritesheet 2 (portals)
        *rel_index = index - SPRITESHEET2_START;
        return &spriteSheet2;
    }

    if (index < ANIMATEDSHEET_START) {
        // Return spritesheet 3 (2.0 objects)
        *rel_index = index - SPRITESHEET3_START;
        return &spriteSheet3;
    }

    // Return spritesheet 4 (animated objects)
    *rel_index = index - ANIMATEDSHEET_START;
    return &animatedSheet;
}

C2D_SpriteSheet *get_sprite_sheet_ex(int index, int *rel_index) {
    return get_sprite_sheet(index, rel_index);
}

const SlotFrames* find_slot_frames(const GameObject* obj, int slot) {
    for (int i = 0; i < obj->slot_count; i++) {
        if (obj->slot_frames[i].slot == slot) return &obj->slot_frames[i];
    }
    return NULL;
}

int get_child_group(const GameObject* obj, int child_index) {
    for (int g = 0; g < obj->group_count; g++) {
        int end = obj->groups[g].start + obj->groups[g].count;
        if (child_index >= obj->groups[g].start && child_index < end)
            return g;
    }
    return -1;
}

const Animation* get_animation_for_object(int id) {
    switch (id) {
        case 918: return &animations[ANIM_GJBEAST01_BITE];
        case 919: return &animations[ANIM_BLACKSLUDGE_LOOP];
        case 1327: return &animations[ANIM_GJBEAST02_IDLE01];
        case 1328: return &animations[ANIM_GJBEAST03_IDLE01];
        default:  return NULL;
    }
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
    if ((color.r | color.g | color.b) == 0) return white;
    
    return color;
}

// Gets p1 color accounting for p1 and p2 being black
Color get_p2_if_black(Color color) {
    // Check if p1 is black
    if ((color.r | color.g | color.b) == 0) {
        // If p1 is also black, return white
        if ((p2_color.r | p2_color.g | p2_color.b) == 0) {
            return white;
        }
        // Else just return p2
        return p2_color;
    }
    
    return color;
}

// Gets p2 color accounting for p1 and p2 being black
Color get_p1_if_black(Color color) {
    // Check if p2 is black
    if ((color.r | color.g | color.b) == 0) {
        // If p1 is also black, return white
        if ((p1_color.r | p1_color.g | p1_color.b) == 0) {
            return white;
        }
        // Else return p1
        return p1_color;
    }
    
    return color;
}

void set_player_colors(Color p1, Color p2, Color glow) {
    p1_color = p1;
    p2_color = p2;
    glow_color = glow;
}

// This might make sprite making faster, im not sure thought
void cache_all_sprites() {
    for (int id = 0; id < GAME_OBJECT_COUNT; id++) {
        const GameObject* obj = &game_objects[id];

        // Skip if object has no texture
        if (obj->texture < 0 && obj->child_count == 0) continue;

        // parent template (if object has a parent textur
        if (obj->texture >= 0) {
            int tex;
            C2D_SpriteSheet *sheet = get_sprite_sheet(obj->texture, &tex);

            C2D_SpriteFromSheet(&sprite_templates[id].parent_template, *sheet, tex);
            C3D_TexSetFilter(sprite_templates[id].parent_template.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&sprite_templates[id].parent_template, 0.5f, 0.5f);
        }

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

float mirror_angle(float angle, bool hflip, bool vflip) {
    if (hflip && vflip) {
        angle += 180.0f;
    } else if (hflip) {
        angle = 180.0f - angle;
    } else if (vflip) {
        angle = -angle;
    }

    return normalize_angle(angle);
}

// Returns true if the object is a invisible object
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

int get_glow_channel(int obj);

inline int get_color_channel(int col_type, int obj, const GameObject *game_obj) {
    int obj_id = objects.id[obj];
    int col_channel = game_obj->base_color;
    if (col_type == COLOR_TYPE_GLOW) {
        col_channel = get_glow_channel(obj);
        if (col_channel != CHANNEL_OBJ_BLENDING) return col_channel;
    } 
    
    // Check for the presence of 1.9 color channel
    if (objects.v1p9_col_channel[obj]) {
        // If pulserods, use base instead of detail
        if (obj_id >= 15 && obj_id <= 17) {
            if (col_type != COLOR_TYPE_DETAIL) col_channel = objects.v1p9_col_channel[obj];
        } else {
            if (col_type == COLOR_TYPE_DETAIL) col_channel = objects.v1p9_col_channel[obj];
        }
    } else {
        // 2.0 color channels, here for 1.9 levels that got updated in 2.0 (and for making 1.9 levels in 2.2)
        if (objects.col_channel[obj]) {
            if (col_type != COLOR_TYPE_DETAIL) {
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

    return col_channel;
}

// Baby's first reverse engineered function

const float leftFadeBound = (SCREEN_WIDTH_AREA/2) - 75.f;
const float leftFadeWidth = leftFadeBound - 30;
const float rightFadeBound = leftFadeBound + 110;
const float rightFadeWidth = (SCREEN_WIDTH_AREA) - (leftFadeBound + 190);

float get_fading_obj_fade(int obj, float right_edge, float *glow_out) {
    if (!state.dead) {
        // Offset fade checks slightly so invisible blocks
        // begin fading before reaching the actual boundary
        float objX = objects.x[obj];
        float marginX = objX;
        if (objX <= state.camera_x_middle) {
            marginX += 0;//object->m_fadeMargin;
        } else {
            marginX -= 0;//object->m_fadeMargin;
        }
        objX = marginX;

        float halfCameraWidth = (SCREEN_WIDTH_AREA / 2);
        float camX = state.camera_x;
        
        // Additional screen-edge fade so objects near the
        // far edges of the screen become less visible
        float edgeFactor;
        float distanceFromCenter;
        if (objX <= halfCameraWidth + camX) {
            // Left fade
            edgeFactor = 0.014285714f;
            distanceFromCenter = ((halfCameraWidth + camX) - objX);
        } else {
            // Right fade
            edgeFactor = 0.02f;
            distanceFromCenter = (objX - camX) - halfCameraWidth;
        }
        
        // Convert edge distance into a normalized visibility factor
        float visibilityScale = (halfCameraWidth - distanceFromCenter) * edgeFactor;
        float edgeVisibilityFactor = CLAMP(visibilityScale, 0.0f, 1.0f);

        // Compute fade distance from the invisible region bounds
        float distanceFromFade;
        float fadeWidth;

        if (marginX <= camX + rightFadeBound) {
            // Left fade
            distanceFromFade = (camX + leftFadeBound) - marginX;
            fadeWidth = leftFadeWidth;
        } else {
            // Right fade
            distanceFromFade = (marginX - camX) - rightFadeBound;
            fadeWidth = rightFadeWidth;
        }

        // Set a minimum of 1
        if (fadeWidth <= 1.0f) {
            fadeWidth = 1.0f;
        }
        
        // Minimum opacity is 5%
        float fadeAlpha = CLAMP(distanceFromFade / fadeWidth, 0.0f, 1.0f);
        int objectOpacity = (fadeAlpha * 0.95f + 0.05f) * 255;
        
        int edgeVisibility = edgeVisibilityFactor * 255;
        if (objectOpacity >= edgeVisibility) {
            objectOpacity = edgeVisibility;
        }

        int glowOpacity = (fadeAlpha * 0.85f + 0.15f) * 255;
        if (glowOpacity >= edgeVisibility) {
            glowOpacity = edgeVisibility;
        }

       *glow_out = glowOpacity / 255.f;
        return objectOpacity / 255.f;
    }

    return 1.f;
}

// Get the glow color channel
int get_glow_channel(int obj) {
    if (object_fades(obj)) {
        return CHANNEL_INVISIBLE_GLOW;
    }

    int id = objects.id[obj];
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
        case 918:
        case 1327:
        case 1328:
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
        case YELLOW_PAD:
        case YELLOW_ORB:
            return CHANNEL_YELLOW_GLOW_INTERNAL;
        case BLUE_PAD:
        case BLUE_ORB:
            return CHANNEL_BLUE_GLOW;
        case PINK_PAD:
        case PINK_ORB:
            return CHANNEL_PINK_GLOW;
        case 200:
        case 201:
        case 202:
        case 203:
            return CHANNEL_WHITE_GLOW;
        case 397:
        case 398:
        case 399:
        case 675:
        case 676:
        case 677:
            return CHANNEL_LBG;

    }
    return CHANNEL_OBJ_BLENDING;
}

int get_coin_texture(int tex, int ticks) {
    return tex + ((level_frame / ticks) & 0b11);;
}

// Some objects have a randomized texture at level load, get those
int get_obj_random_layer(int obj, int id) {
    int tex = game_objects[id].texture;
    switch (id) {
        case 9:
            int offset = objects.random[obj] & 0b11;
            if (offset == 3) offset = 0;
            
            if (offset > 0) offset += 3;

            return tex + offset;
        case 135:
            return tex + (objects.random[obj] & 0b11);
        
        case SECRET_COIN:
            return get_coin_texture(tex + (state.custom_level ? (is_coin_collected(obj) ? 8 : 4) : (is_coin_collected(obj) ? 12 : 0)), 26);
    }
    return -1;
}

// Deco saws rotate slower than normal saws. If not a saw, rotation speed is just 0
float get_rotation_speed(int obj) {
    switch (objects.id[obj]) {
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
        case 997:
        case 998:
        case 999:
        case 1000:
        case 1055:
        case 1056:
        case 1057:
        case GREEN_ORB:
            return 180.f;
        case 1019:
            return 180.f + map_range(objects.random[obj] & 0xff, 0, 255, -10, 10);
        case 1020:
            return 100.f + map_range(objects.random[obj] & 0xff, 0, 255, -10, 10);
        case 1021:
            return 80.f + map_range(objects.random[obj] & 0xff, 0, 255, -10, 10);
        case 1058:
        case 1059:
        case 1060:
        case 1061:
            return 300.f;
    }
    return 0.f;
}

// Map amplitude pulsing to ranges
float get_object_pulse(float amplitude, int id, int layer) {
     // No pulse if one of those is 0
    amplitude *= music_volume > 0 && global_volume > 0;
    amplitude = MAX(0.1f, amplitude); // Cap at 0.1
    switch (id) {
        case YELLOW_ORB:
        case BLUE_ORB:
        case PINK_ORB:
        case GREEN_ORB:
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

static bool object_has_pulse(int id) {
    switch (id) {
        case 15:
        case 16:
        case 17:
        case YELLOW_ORB:
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 60:
        case BLUE_ORB:
        case 132:
        case 133:
        case 136:
        case PINK_ORB:
        case 148:
        case 149:
        case 150:
        case 236:
        case 405:
        case 460:
        case 494:
        case 495:
        case 496:
        case 497:
        case GREEN_ORB:
            return true;
        default:
            return false;
    }
}

int get_color_type(const GameObject *game_obj, int obj, int col_type) {
    // Check for the presence of 1.9 color channel
    if (objects.v1p9_col_channel[obj]) {
        col_type = COLOR_TYPE_BASE;
    } else {
        if (col_type == COLOR_TYPE_DETAIL) {
            if (!obj_has_main(game_obj)) {
                col_type = COLOR_TYPE_BASE;
            }
        }
    }
    return col_type;
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
    float cos_r = lut_cos(rad);
    float sin_r = lut_sin(rad);

    int flip_x_mult = (flip_x ? -1 : 1);
    int flip_y_mult = (flip_y ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = sin_r;
    float m11 = -cos_r;

    float obj_scale_x = objects.scale_x[obj_game];
    float obj_scale_y = objects.scale_y[obj_game];

    float sx = scale * flip_x_mult * obj_scale_x;
    float sy = scale * flip_y_mult * obj_scale_y;

    // get anim for this object
    const AnimFrame* anim_keyframe = NULL;
    if (obj->animation_type == ANIMATION_MOVEMENT && obj->group_count > 0) {
        const Animation* anim = get_animation_for_object(id);
        if (anim && anim->frame_count > 0) {
            float time = frame_timer * anim->fps;
            anim_keyframe = &anim->frames[(int)time % anim->frame_count];
        }
    }

    if (sprite_count >= MAX_SPRITES - 1) return;

    if (id == TEXT_OBJECT) {
        TextObject *text_obj = get_text_object(obj_game);
        if (text_obj->len > 0) {
            if (!text_obj->layout_done) {
                text_obj->glyph_count = text_object_layout(level_font, text_obj->text, text_obj->glyphs, MAX_TEXT_LEN);
                text_obj->layout_done = 1;
            }

            for (int i = 0; i < text_obj->glyph_count; i++) {
                if (sprite_count >= MAX_SPRITES - 1) break;

                const TextGlyphPlacement *place = &text_obj->glyphs[i];
                SpriteObject *vo = &viewable_objects[sprite_count];

                vo->hidden = false;

                float local_x = place->x * flip_x_mult;
                float local_y = place->y * flip_y_mult;

                float rot_x = local_x * cos_r - local_y * sin_r;
                float rot_y = local_x * sin_r + local_y * cos_r;

                C2D_SpriteFromSheet(&vo->spr, level_font_sheet ? level_font_sheet : bigFont_sheet, place->sprite);
                C3D_TexSetFilter(vo->spr.image.tex, GPU_LINEAR, GPU_LINEAR);
                C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);
                C2D_SpriteSetPos(&vo->spr, x + rot_x * scale * obj_scale_x, y + rot_y * scale * obj_scale_y);
                C2D_SpriteSetScale(&vo->spr, TEXT_OBJECT_SCALE * sx, TEXT_OBJECT_SCALE * sy);
                C2D_SpriteSetRotation(&vo->spr, rad);

                vo->obj = obj_game;
                vo->layer = 0;
                vo->col_type = get_color_type(obj, obj_game, obj->color_type);
                vo->opacity = obj->opacity;
                vo->col_channel = get_color_channel(obj->color_type, obj_game, obj);
                calc_quad_params(vo);

                sprite_count++;
            }
        }
        return;
    }

    // Spawn parent, skip if no texture
    if (obj->texture >= 0) {
        SpriteObject *vo = &viewable_objects[sprite_count];

        vo->hidden = false;

        float local_x = obj->x * flip_x_mult;
        float local_y = obj->y * flip_y_mult;

        float rot_x = local_x * m00 + local_y * m01;
        float rot_y = local_x * m10 + local_y * m11;

        float p_x = x + rot_x * scale * obj_scale_x;
        float p_y = y + rot_y * scale * obj_scale_y;
        
        if (obj->animation_type == ANIMATION_FRAME_SWAP && obj->frame_count > 0) {
            const SlotFrames* slot_frames = find_slot_frames(obj, 0);
            if (slot_frames) {
                float time = frame_timer * slot_frames->fps;
                int index = (int)time % slot_frames->count;
                const SwapFrame* swap_frame = &obj->swap_frames[slot_frames->start + index];

                int rel_index;
                C2D_SpriteSheet *sheet = get_sprite_sheet(swap_frame->texture, &rel_index);
                C2D_SpriteFromSheet(&vo->spr, *sheet, rel_index);
                C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);
                
                sx *= (swap_frame->flip_x ? -1 : 1);
                sy *= (swap_frame->flip_y ? -1 : 1);
            } else {
                vo->spr = sprite_templates[id].parent_template;
            }
        } else {
            int random_layer = get_obj_random_layer(obj_game, id);
            if (random_layer < 0) {
                vo->spr = sprite_templates[id].parent_template;
            } else {
                int rel_index;
                C2D_SpriteSheet *sheet = get_sprite_sheet(random_layer, &rel_index);
                C2D_Sprite rnd = { 0 };
                vo->spr = rnd;
                C2D_SpriteFromSheet(&vo->spr, *sheet, rel_index);
                C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);
            }
        }

        float pulse_scale = get_object_pulse(amplitude, id, 0);

        C2D_SpriteSetPos(&vo->spr, p_x, p_y);
        C2D_SpriteSetScale(&vo->spr, sx * pulse_scale, sy * pulse_scale);
        C2D_SpriteSetRotation(&vo->spr, rad);

        vo->obj = obj_game;
        vo->layer = 0;
        vo->col_type = get_color_type(obj, obj_game, obj->color_type);
        vo->opacity = obj->opacity;
        vo->col_channel = get_color_channel(obj->color_type, obj_game, obj);
        calc_quad_params(vo);

        sprite_count++;
    }

    // Skip if no glow frame
    if (settingsState.glowEnabled && obj->glow_frame >= 0) {
        if (sprite_count >= MAX_SPRITES - 1) return;

        SpriteObject *vo = &viewable_objects[sprite_count];

        vo->hidden = false;

        vo->spr = sprite_templates[id].glow_template;

        float pulse_scale = get_object_pulse(amplitude, id, 1);

        C2D_SpriteSetPos(&vo->spr, x, y);
        C2D_SpriteSetScale(&vo->spr, sx * pulse_scale, sy * pulse_scale);
        C2D_SpriteSetRotation(&vo->spr, rad);

        vo->obj = obj_game;
        vo->layer = 1;
        vo->col_type = COLOR_TYPE_GLOW;
        vo->opacity = obj->opacity;
        vo->blending = true;
        vo->col_channel = get_color_channel(COLOR_TYPE_GLOW, obj_game, obj);
        calc_quad_params(vo);
        sprite_count++;
    }

    // Spawn children
    for (int i = 0; i < obj->child_count; i++) {
        const ChildSprite* c = &obj->children[i];
        
        if (sprite_count >= MAX_SPRITES - 1) return;
        
        // Skip if no texture
        if (c->texture >= 0) {    
            SpriteObject *vo = &viewable_objects[sprite_count];

            vo->hidden = false;

            float c_local_x = c->x * flip_x_mult;
            float c_local_y = c->y * flip_y_mult;

            float c_rot_x = c_local_x * m00 + c_local_y * m01;
            float c_rot_y = c_local_x * m10 + c_local_y * m11;

            float c_x = x + c_rot_x * scale * obj_scale_x;
            float c_y = y + c_rot_y * scale * obj_scale_y;

            int c_flip_x_mult = (c->flip_x ? -1 : 1);
            int c_flip_y_mult = (c->flip_y ? -1 : 1);

            float c_rot = C3D_AngleFromDegrees(c->rot) + rad;
            float c_sx = c->scale_x * sx;
            float c_sy = c->scale_y * sy;

            // handle movement anims
            if (anim_keyframe) {
                int group = get_child_group(obj, i);
                if (group >= 0) {
                    const AnimSprite* anim_sprite = NULL;
                    for (int k = 0; k < anim_keyframe->sprite_count; k++) {
                        if (anim_keyframe->sprites[k].child_slot == group) {
                            anim_sprite = &anim_keyframe->sprites[k];
                            break;
                        }
                    }

                    if (anim_sprite) {
                        float a_local_x = anim_sprite->x * flip_x_mult;
                        float a_local_y = anim_sprite->y * flip_y_mult;

                        float a_rot_x = a_local_x * m00 + a_local_y * m01;
                        float a_rot_y = a_local_x * m10 + a_local_y * m11;

                        c_x = x + a_rot_x * scale * obj_scale_x;
                        c_y = y + a_rot_y * scale * obj_scale_y;

                        c_rot = C3D_AngleFromDegrees(anim_sprite->rot) * (flip_x_mult * flip_y_mult) + rad;

                        c_sx *= anim_sprite->scale_x;
                        c_sy *= anim_sprite->scale_y;

                        c_flip_x_mult ^= anim_sprite->flip_x;
                        c_flip_y_mult ^= anim_sprite->flip_y;
                    }
                }
            }
            // handle frame swap anims
            if (obj->animation_type == ANIMATION_FRAME_SWAP && obj->frame_count > 0) {
                const SlotFrames* slot_frames = find_slot_frames(obj, i + 1);
                if (slot_frames) {
                    float time = frame_timer * slot_frames->fps;
                    int index = (int)time % slot_frames->count;
                    const SwapFrame* swap_frame = &obj->swap_frames[slot_frames->start + index];

                    int rel_index;
                    C2D_SpriteSheet *sheet = get_sprite_sheet(swap_frame->texture, &rel_index);
                    C2D_SpriteFromSheet(&vo->spr, *sheet, rel_index);
                    C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);

                    c_sx *= (swap_frame->flip_x ? -1 : 1);
                    c_sy *= (swap_frame->flip_y ? -1 : 1);
                } else {
                    if (!sprite_templates[id].child_templates) continue;
                    vo->spr = sprite_templates[id].child_templates[i];
                }
            } else {
                if (!sprite_templates[id].child_templates) continue;
                vo->spr = sprite_templates[id].child_templates[i];
            }

            float pulse_scale = get_object_pulse(amplitude, id, i + 2);

            C2D_SpriteSetPos(&vo->spr, c_x, c_y);
            if (id < 15 || id > 17) {
                C2D_SpriteSetScale(&vo->spr, c_sx * c_flip_x_mult * pulse_scale,
                                          c_sy * c_flip_y_mult * pulse_scale);
                C2D_SpriteSetRotation(&vo->spr, c_rot);
            } else {
                C2D_SpriteSetScale(&vo->spr, fabsf(c_sx * c_flip_x_mult * pulse_scale),
                                          fabsf(c_sy * c_flip_y_mult * pulse_scale));
            }

            vo->obj = obj_game;
            vo->layer = i + 2;
            vo->col_type = get_color_type(obj, obj_game, c->color_type);
            vo->opacity = c->opacity;
            vo->col_channel = get_color_channel(c->color_type, obj_game, obj);
            calc_quad_params(vo);
            sprite_count++;
        }
    }
}

static inline uint32_t make_sort_key(SpriteObject *s)
{
    const int obj = s->obj;

    // Player sprite is -1 so handle it there
    if (obj == -1) {
        return ((4 + 8) << 18) | (2 << 16) | (255 << 8) | 128;
    }

    const int id = objects.id[obj];
    const GameObject *game_obj = &game_objects[id];

    int zlayer = objects.zlayer[obj];
    int zorder = objects.zorder[obj];

    bool blending;
    // Blending makes zlayer one 
    if (obj_has_main(game_obj) && obj_has_detail(game_obj)) {
        bool blending_main = (objects.col_channel[obj] > 0 && (channels[get_col_channel_index(objects.col_channel[obj])].blending ^ ((zlayer & 1) == 0)));
        bool blending_detail = (objects.detail_col_channel[obj] > 0 && (channels[get_col_channel_index(objects.detail_col_channel[obj])].blending ^ ((zlayer & 1) == 0)));
        blending = blending_main && blending_detail;
    } else {
        int col_channel = s->col_channel;
        blending = col_channel > 0 && (channels[get_col_channel_index(col_channel)].blending ^ ((zlayer & 1) == 0));
    }

    // If layer is a glow layer or it has blending, decrement it
    if (s->layer == 1 || blending) {
        zlayer--;
    }

    int child_z = 0;
    int tex = game_obj->texture;

    // If layer is a glow layer, it does something for sure
    if (s->layer > 1) {
        const ChildSprite *child = &game_obj->children[s->layer - 2];
        child_z = child->z - 1;
        tex = child->texture;
        zlayer += child->z_layer_offset;
    }
    
    // Glow layers always use spritesheet 2 (only for sorting purposes)
    int sheet;
    if (s->layer == 1) {
        sheet = 0;
    } else {
        sheet = tex < SPRITESHEET2_START || tex >= SPRITESHEET3_START ? 1 : 2;
        // Some animated object are in sheet 3
        switch (id) {
            case 918:
            case 1327:
            case 1328:
            case 920:
            case 921:
            case 923:
            case 924:
                sheet = 3;
                break;
        }
    }

    // Move the pulserod ball
    if (id >= 15 && id <= 17 && s->layer == 2) {
        zlayer += 2;
    } 

    s->zlayer = zlayer;

    // Pack all variables into a nice 32 bit variable
    uint32_t zl = (uint32_t)(zlayer + 8);     // fits in 6 bits
    uint32_t zs = (uint32_t)(3 - sheet);          // fits in 2 bit
    uint32_t zo = (uint32_t)(zorder + 128);   // fits in 8 bits
    uint32_t cz = (uint32_t)(child_z + 128);  // fits in 8 bits

    return (zl << 18) | (zs << 16) | (zo << 8) | cz;
}

void sort_viewable_objects(SpriteObject **objects, int count) {
    if (count <= 1) return;

    for (int i = 0; i < count; i++) {
        buf_a[i].obj = objects[i];
        buf_a[i].key = make_sort_key(objects[i]);
    }

    SortItem *src = buf_a;
    SortItem *dst = buf_b;

    for (int pass = 0; pass < 3; pass++) {
        uint16_t buckets[256] = {0}; // Crum buckets, speak to da weeb, began duh uh oh oh, oh, oh oh oh oh wiguwiguwi
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

    const GameObject *obj = &game_objects[id];
    if (obj->texture >= 0) count++;
    
    for (size_t c = 0; c < obj->child_count; c++) {
        if (obj->children[c].texture >= 0) count++;
    }
    return count;
}

static int get_object_sprite_total(int obj) {
    int id = objects.id[obj];
    if (id < 0 || id >= GAME_OBJECT_COUNT) return 0;

    const GameObject *game_object = &game_objects[id];
    int count = get_object_layers(objects.id[obj]);
    if (game_object->glow_frame >= 0) count++;

    if (objects.id[obj] == TEXT_OBJECT) {
        TextObject *text_obj = get_text_object(obj);
        if (text_obj->len > 0) {
            if (!text_obj->layout_done) {
                text_obj->glyph_count = text_object_layout(level_font, text_obj->text, text_obj->glyphs, MAX_TEXT_LEN);
                text_obj->layout_done = 1;
            }
            count += text_obj->glyph_count;
        }
    }

    return count;
}

float obj_edge_fade(float x, int right_edge) {
    if (x < 0 || x > right_edge)
        return 0;
    else if (x < FADE_WIDTH)
        return 255.0f * (x / FADE_WIDTH);
    else if (x > right_edge - FADE_WIDTH)
        return 255.0f * ((right_edge - x) / FADE_WIDTH);
    else
        return 255;
}

float get_xy_fade_offset(float x, int right_edge) {
    float fade = obj_edge_fade(x, right_edge);
    return (255 - fade) / 2;
}

float get_in_scale_fade(float x, int right_edge) {
    float fade = obj_edge_fade(x, right_edge);
    return (fade / 255.f);
}

float get_out_scale_fade(float x, int right_edge) {
    float fade = 255 - obj_edge_fade(x, right_edge);
    return 1 + ((fade / 255.f) / 2);
}

// Some objects dont change opacity on fade transitions
int get_obj_opacity(int obj, float x) {
    if (objects.flags[obj] & FLAG_DONT_FADE) return 255;

    float opacity = obj_edge_fade(x, SCREEN_WIDTH / SCALE);
    bool blending;

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
            if (objects.transition_applied[obj] == FADE_SIMPLE) opacity = 255;
            break;
            
        case 211:
            blending = channels[get_col_channel_index(objects.col_channel[obj])].blending;
            if (!blending && objects.transition_applied[obj] == FADE_SIMPLE) opacity = 255;
            break;
        case 207:
        case 208:
        case 209:
        case 210:
        case 212:
        case 213:
        case 693:
        case 694:
        case 331:
        case 333:
            blending = channels[get_col_channel_index(objects.detail_col_channel[obj])].blending;
            if (!blending && objects.transition_applied[obj] == FADE_SIMPLE) opacity = 255;
            break;
    }

    return opacity;
}

// Handle complex fading transitions
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
                    objects.transition_applied[obj] = FADE_UP_SLOW_LEFT;
                } else {
                    objects.transition_applied[obj] = FADE_DOWN_SLOW_LEFT;
                }
            }
            break;
        case FADE_CIRCLE_RIGHT:
            if (calc_x > (SCREEN_WIDTH / SCALE / 2)) {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transition_applied[obj] = FADE_UP_SLOW_RIGHT;
                } else {
                    objects.transition_applied[obj] = FADE_DOWN_SLOW_RIGHT;
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

void get_fade_vars(int obj, float x, float *fade_x, float *fade_y, float *fade_scale) {
    if (objects.flags[obj] & FLAG_DONT_ENTER) return;

    switch (objects.transition_applied[obj]) {
        case FADE_SIMPLE:
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
        case FADE_UP_SLOW_LEFT:
            *fade_x = -get_xy_fade_offset(x, SCREEN_WIDTH / SCALE);
            *fade_y = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) / 3;
            break;
        case FADE_UP_SLOW_RIGHT:
            *fade_x = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE);
            *fade_y = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) / 3;
            break;
        case FADE_UP_STATIONARY:
            *fade_y = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) / 3;
            break;
        case FADE_DOWN_SLOW_LEFT:
            *fade_x = -get_xy_fade_offset(x, SCREEN_WIDTH / SCALE);
            *fade_y = -get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) / 3;
            break;
        case FADE_DOWN_SLOW_RIGHT:
            *fade_x = get_xy_fade_offset(x, SCREEN_WIDTH / SCALE);
            *fade_y = -get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) / 3;
            break;
        case FADE_DOWN_STATIONARY:
            *fade_y = -get_xy_fade_offset(x, SCREEN_WIDTH / SCALE) / 3;
            break;
    }
}

float get_special_fading_vars(int obj, float fade_val) {
    if (objects.flags[obj] & FLAG_DONT_ENTER) return 0;

    if (objects.transition_applied[obj] == FADE_DOWN_STATIONARY || objects.transition_applied[obj] == FADE_UP_STATIONARY) {
        if (fade_val < 255) {
            float calc_x = objects.x[obj] - state.camera_x;
            if (calc_x > (SCREEN_WIDTH / SCALE) / 2) {
                return (SCREEN_WIDTH / SCALE - FADE_WIDTH) - calc_x;
            } else {
                return FADE_WIDTH - calc_x;
            }
        }
    }
    return 0;
}

void change_blending(bool blending) {
    // If changing blending to the same state, do nothing a state change its not worth it
    if (blending == blending_state) return;

    if (blending) {
        C2D_Flush();
        C3D_AlphaBlend(
            GPU_BLEND_ADD, GPU_BLEND_ADD,
            GPU_SRC_ALPHA, GPU_ONE,
            GPU_ONE, GPU_ZERO
        );

        C2D_Prepare();
        C3D_TexEnv *env = C3D_GetTexEnv(4);
        C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_PREVIOUS, GPU_PREVIOUS, 0);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
    } else {
        C2D_Flush();
        C3D_AlphaBlend(
            GPU_BLEND_ADD, GPU_BLEND_ADD, 
            GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, 
            GPU_ONE, GPU_ZERO);
        
        C2D_Prepare();
        C3D_TexEnv *env = C3D_GetTexEnv(4);
        C3D_TexEnvInit(env);
    }

    blending_state = blending;
}

void draw_background(float x, float y) {
    C2D_ImageTint tint = { 0 };

    Color col = channels[get_col_channel_index(CHANNEL_BG)].color;

    // If flash is happening, use lbg
    if (state.flash_data.use_lbg) col = channels[get_col_channel_index(CHANNEL_LBG_NOLERP)].color;

    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    float offset = 512 * BACKGROUND_SCALE;

    float calc_x = positive_fmodf(x, offset);
    float draw_y = -y;

    int bg_id = level_info.background_id;
    int bg_idx = bg_id & 0b11;

    // guard against an out-of-range sprite index for the loaded sheet
    if (bgSheet == NULL || (size_t)bg_idx >= C2D_SpriteSheetCount(bgSheet)) return;

    for (int i = -1; i < 3; i++) {
        C2D_Sprite bg = { 0 };
        // Calculate position for each tile
        float draw_x = -calc_x + i * offset;

        
        C2D_SpriteFromSheet(&bg, bgSheet, bg_idx);
        C3D_TexSetFilter(bg.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetPos(&bg, (int)draw_x, (int)draw_y);
        C2D_SpriteSetScale(&bg, BACKGROUND_SCALE, BACKGROUND_SCALE);
        C2D_DrawSpriteTinted(&bg, &tint);
    }
}

const int ground_indexes[G_COUNT] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14};

void draw_ground(float cam_x, float cam_y, float y, bool is_ceiling, int screen_width) {
    change_blending(false);
    int mult = (is_ceiling ? -1 : 1);
    
    C2D_ImageTint tint = { 0 };
    Color col = channels[get_col_channel_index(CHANNEL_GROUND)].color;
    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    int ground_id = ground_indexes[level_info.ground_id];
    bool has_l2 = level_info.ground_id >= 7;

    C2D_Sprite ground = { 0 };
    C2D_SpriteFromSheet(&ground, groundSheet, ground_id);
    C3D_TexSetFilter(ground.image.tex, GPU_LINEAR, GPU_LINEAR);

    if (is_ceiling) y += GROUND_SIZE;

    float g_y = y;

    if (has_l2) {
        if (!is_ceiling) {
            g_y -= GROUND_SIZE - ground.image.subtex->height - 1;
        } else {
            g_y -= 1;
        }
    }

    // First draw the ground
    float calc_x = 0 - positive_fmodf(cam_x, GROUND_SIZE);
    float calc_y = SCREEN_HEIGHT - ((y - cam_y));
    float ground_calc_y = SCREEN_HEIGHT - ((g_y - cam_y));

    for (float i = -GROUND_SIZE; i < (screen_width / SCALE) + GROUND_SIZE; i += GROUND_SIZE) {
        C2D_SpriteSetPos(&ground, calc_x + i, ground_calc_y);
        C2D_SpriteSetScale(&ground, 1.f, mult);
        C2D_DrawSpriteTinted(&ground, &tint);
    }

    if (has_l2) {
        col = channels[get_col_channel_index(CHANNEL_GROUND_2)].color;
        C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

        C2D_Sprite ground2 = { 0 };
        C2D_SpriteFromSheet(&ground2, groundSheet, ground_id + 1);
        C3D_TexSetFilter(ground2.image.tex, GPU_LINEAR, GPU_LINEAR);

        float g2_y = y;
        if (is_ceiling) g2_y -= GROUND_SIZE - ground2.image.subtex->height;

        ground_calc_y = SCREEN_HEIGHT - ((g2_y - cam_y));
        for (float i = -GROUND_SIZE; i < (screen_width / SCALE) + GROUND_SIZE; i += GROUND_SIZE) {
            C2D_SpriteSetPos(&ground2, calc_x + i, ground_calc_y);
            C2D_SpriteSetScale(&ground2, 1.f, mult);
            C2D_DrawSpriteTinted(&ground2, &tint);
        }
    }

    C2D_PlainImageTint(&tint, C2D_Color32(0, 0, 0, 100), 1.f);
    C2D_Sprite ground_shadow = { 0 };

    C2D_SpriteFromSheet(&ground_shadow, ui_sheet, 361);
    C3D_TexSetFilter(ground_shadow.image.tex, GPU_LINEAR, GPU_LINEAR);

    // Left shadow
    C2D_SpriteSetPos(&ground_shadow, 0, calc_y);
    C2D_SpriteSetScale(&ground_shadow, 1.f, 1.f);
    C2D_DrawSpriteTinted(&ground_shadow, &tint);

    // Right shadow
    C2D_SpriteSetPos(&ground_shadow, screen_width / SCALE, calc_y);
    C2D_SpriteSetCenter(&ground_shadow, 1.f, 0.f);
    C2D_SpriteSetScale(&ground_shadow, -1.f, 1.f);
    C2D_DrawSpriteTinted(&ground_shadow, &tint);

    int line_chan = get_col_channel_index(CHANNEL_LINE);
    // Then draw the line
    if (channels[line_chan].blending) {
        change_blending(true);
    }

    col = channels[line_chan].color;
    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    float line_offset = -((GROUND_SIZE / 2) - (LINE_HEIGHT / 2)) * mult;
    C2D_Sprite line = { 0 };
    C2D_SpriteFromSheet(&line, groundSheet, 0);
    C3D_TexSetFilter(line.image.tex, GPU_LINEAR, GPU_LINEAR);
    C2D_SpriteSetCenter(&line, 0.5f, 0.5f);
    C2D_SpriteSetPos(&line, screen_width / SCALE / 2, (GROUND_SIZE / 2) + calc_y + line_offset);
    C2D_DrawSpriteTinted(&line, &tint);

    if (channels[line_chan].blending) {
        change_blending(false);
    }
}

float complete_text_elapsed = 0;
void draw_end_wall(float delta) {  
    float calc_x = ((level_info.wall_x - state.camera_x));
    float calc_y =  positive_fmodf(state.camera_y, 30) + 15;  
    if (level_info.wall_y > 0) {
        // Draw each wall block
        for (float i = -30; i < SCREEN_HEIGHT_AREA + 30; i += 30) {
            C2D_Sprite block = { 0 };
            C2D_SpriteFromSheet(&block, spriteSheet, game_objects[2].texture);
            C3D_TexSetFilter(block.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&block, 0.5f, 0.5f);
            C2D_SpriteSetPos(&block, get_mirror_x(calc_x, state.mirror_factor), calc_y + i);
            C2D_SpriteSetRotationDegrees(&block, adjust_angle(270, 0, state.mirror_mult < 0));
            C2D_DrawSprite(&block);
        }

        change_blending(true);

        C2D_ImageTint tint = { 0 };
        Color col = get_p2_if_black(p1_color);
        C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

        // Draw glow
        for (float i = -30; i < SCREEN_HEIGHT_AREA + 30; i += 30) {
            C2D_Sprite glow = { 0 };
            C2D_SpriteFromSheet(&glow, spriteSheet, game_objects[503].texture);
            C3D_TexSetFilter(glow.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&glow, 0.5f, 0.5f);
            C2D_SpriteSetPos(&glow, get_mirror_x(calc_x - 25, state.mirror_factor), calc_y + i);
            C2D_SpriteSetRotationDegrees(&glow, adjust_angle(270, 0, state.mirror_mult < 0));
            C2D_DrawSpriteTinted(&glow, &tint);
        }
    }   
    change_blending(false);
}

void draw_attempt_text() {
    int attempts = state.current_data.attempts;

    float calc_x = (state.attempt_text_pos.x - state.camera_x);
    float calc_y = SCREEN_HEIGHT - ((state.attempt_text_pos.y - state.camera_y));  

    if (calc_x > -200) {
        C2D_SpriteSheet *font_sheet = level_font_sheet ? &level_font_sheet : &bigFont_sheet;
        draw_text(level_font, font_sheet, get_mirror_x(calc_x, state.mirror_factor), calc_y, 1, (settingsState.doNot ? -1 : 1), 0.5f, true, "Attempt %d", attempts);
    }
}

bool ensure_render_cache(void) {
    if (render_object_capacity == objects.count) return true;

    // New objects! Reallocate stuff and maintain pointer integrity

    free(object_sprite_start);
    free(object_sprite_count);
    free(object_sprite_cache);

    object_sprite_start = malloc(sizeof(int) * objects.count);
    object_sprite_count = malloc(sizeof(unsigned char) * objects.count);
    
    if (!object_sprite_start || !object_sprite_count) {
        free(object_sprite_start);
        free(object_sprite_count);
        object_sprite_start = NULL;
        object_sprite_count = NULL;
        object_sprite_cache = NULL;
        render_object_capacity = 0;
        return false;
    }

    // Count all layers
    int cache_capacity = 0;
    for (int obj = 0; obj < objects.count; obj++) {
        int count = get_object_sprite_total(obj);
        cache_capacity += count;
        object_sprite_count[obj] = count;
    }

    object_sprite_cache = malloc(sizeof(SpriteObject) * cache_capacity);
    if (!object_sprite_cache) {
        // Today i discovered free ignores NULL
        free(object_sprite_start);
        free(object_sprite_count);
        free(object_sprite_cache);
        object_sprite_start = NULL;
        object_sprite_count = NULL;
        object_sprite_cache = NULL;
        render_object_capacity = 0;
        return false;
    }

    // Get offsets of each object
    int sprite_offset = 0;
    for (int obj = 0; obj < objects.count; obj++) {
        object_sprite_start[obj] = sprite_offset;
        sprite_offset += get_object_sprite_total(obj);
    }

    current_object_count = 0;
    render_object_capacity = objects.count;
    render_mirror_factor = state.mirror_factor;
    return true;
}

void reset_render_cache(void) {
    free(object_sprite_start);
    free(object_sprite_count);
    free(object_sprite_cache);

    object_sprite_start = NULL;
    object_sprite_count = NULL;
    object_sprite_cache = NULL;
    render_object_capacity = 0;
    current_object_count = 0;
    sprite_count = 0;
    viewable_objects = NULL;
}

static int insert_sorted_object(int obj) {
    if (current_object_count >= MAX_SPRITES) return -1;

    int idx = current_object_count;

    while (idx > 0) {
        int prev = current_objects[idx - 1];

        // keep the list sorted by object index
        if (prev < obj) break;

        current_objects[idx] = prev;
        idx--;
    }

    current_objects[idx] = obj;
    current_object_count++;
    return idx;
}

static void remove_object_at(int index) {
    for (int i = index; i + 1 < current_object_count; i++) {
        current_objects[i] = current_objects[i + 1];
    }
    current_object_count--;
}

static void update_current_objects(void) {
    // Mark everything as unseen, everything that is actually on screen will mark it as seen again
    for (int i = 0; i < current_object_count; i++) {
        objects.flags[current_objects[i]] &= ~FLAG_SEEN;
    }

    int width = ceilf(SCREEN_WIDTH_AREA / SECTION_SIZE);
    int height = ceilf(SCREEN_HEIGHT_AREA / SECTION_SIZE);
    int cam_sx = (int)(state.camera_x / SECTION_SIZE);
    int cam_sy = (int)((state.camera_y - LEVEL_Y_OFFSET) / SECTION_SIZE);

    for (int x = -2; x <= width + 1; x++) {
        for (int y = -2; y <= height + 1; y++) {
            int sx = cam_sx + x;
            int sy = cam_sy + y;

            Section *sec = get_section(sx, sy);
            for (int i = 0; i < sec->object_count; i++) {
                int obj = sec->objects[i];

                float calc_x = objects.x[obj] - state.camera_x;
                float calc_y = SCREEN_HEIGHT - (objects.y[obj] - state.camera_y);

                if (calc_x < -60 || calc_x >= SCREEN_WIDTH / SCALE + 60 || calc_y < -60 || calc_y >= SCREEN_HEIGHT / SCALE + 60) 
                    continue;

                if (!is_valid_object(objects.id[obj]) || objects.flags[obj] & FLAG_TOGGLED) 
                    continue;

                // 0 scale objects are invisible
                if (objects.scale_x[obj] == 0.f || objects.scale_y[obj] == 0.f)
                    continue;

                // This object has just entered the screen
                if (!(objects.flags[obj] & FLAG_VISIBLE)) {
                    objects.flags[obj] |= (FLAG_DIRTY | FLAG_VISIBLE);
                    render_list_changed = true;
                    insert_sorted_object(obj);
                }

                // Mark as seen again
                objects.flags[obj] |= FLAG_SEEN;
            }
        }
    }

    // Remove objects that aren't visible anymore
    for (int i = 0; i < current_object_count;) {
        int obj = current_objects[i];

        // Keep visible
        if (objects.flags[obj] & FLAG_SEEN) {
            i++;
            continue;
        }

        // Not visible anymore, bye
        objects.flags[obj] &= ~FLAG_VISIBLE;
        objects.flags[obj] |= FLAG_DIRTY;
        render_list_changed = true;
        remove_object_at(i);
    }
}

void update_tints() {
    u64 start = svcGetSystemTick();
    for (size_t s = 0; s < sprite_count; s++) {
        SpriteObject *obj = viewable_objects_ptr[s];
        if (obj->obj != -1) {
            int col_channel = obj->col_channel;

            ColorChannel col;

            if (col_channel == CHANNEL_INVISIBLE_GLOW) { // Handle invisible blocks color lerping
                int chan = get_col_channel_index(CHANNEL_LBG_NOLERP);
                col.alpha = channels[chan].alpha;

                Color lbg = channels[chan].color;
                Color p1 = get_white_if_black(p1_color);
                float opacity = objects.opacity[obj->obj];

                if (opacity < 0.8f || state.dead) {
                    col = channels[chan];
                } else {
                    float blendFactor = 1.9f - 1.5f * opacity;
                    float oneMinusFactor = 1.0f - blendFactor;

                    int r = (float)p1.r * oneMinusFactor + (float)lbg.r * blendFactor;
                    int g = (float)p1.g * oneMinusFactor + (float)lbg.g * blendFactor;
                    int b = (float)p1.b * oneMinusFactor + (float)lbg.b * blendFactor;
                    
                    col.color.r = CLAMP(r, 0, 255);
                    col.color.g = CLAMP(g, 0, 255);
                    col.color.b = CLAMP(b, 0, 255);
                    col.blending = true;
                }
            } else if (col_channel == CHANNEL_LIGHTER) {
                // LIGHTER: derive the detail color from the object's main channel + lighter_hsv
                int main_ch = objects.col_channel[obj->obj];
                if (main_ch == 0 || main_ch == CHANNEL_LIGHTER)
                    main_ch = game_objects[objects.id[obj->obj]].base_color;
                main_ch = get_col_channel_index(main_ch);
                col = channels[main_ch];
                col.color = HSV_combine(col.color, lighter_hsv);
                if (objects.main_col_HSV_enabled[obj->obj]) {
                    col.color = HSV_combine(col.color, objects.main_col_HSV[obj->obj]);
                }
                col.blending = false;
                // TODO: pulse interaction
            } else {
                col = channels[get_col_channel_index(col_channel)];
            }
            
            
            int game_object = obj->obj;

            if (obj->col_type != COLOR_TYPE_DETAIL) {
                if (objects.main_col_HSV_enabled[game_object]) {
                    col.color = apply_hsv_to_color(col.color, game_object, true, col_channel);
                }
                objects.main_non_pulse_color[game_object] = col.color;
                if (objects.num_main_pulses[game_object] == 0) {
                    objects.main_color[game_object] = col.color;
                }
            } else {
                if (objects.detail_col_HSV_enabled[game_object]) {
                    col.color = apply_hsv_to_color(col.color, game_object, false, col_channel);
                }
                objects.detail_non_pulse_color[game_object] = col.color;
                if (objects.num_detail_pulses[game_object] == 0) {
                    objects.detail_color[game_object] = col.color;
                }
            }

            switch (obj->col_type) {
                case COLOR_TYPE_GLOW:
                    col.blending = true;
                    break;
                case COLOR_TYPE_WHITE:
                    if (level_is_unrated_online() && objects.id[game_object] == SECRET_COIN) {
                        col.color = (Color) {USER_COIN_UNRATED_R, USER_COIN_UNRATED_G, USER_COIN_UNRATED_B};
                    } else {
                        col.color = (Color) {255,255,255};
                    }
                    break;
            }

            if (obj->col_type != COLOR_TYPE_DETAIL && objects.main_being_pulsed[game_object] && col_channel >= 0) {
                col.color = objects.main_color[game_object];
            } else if (obj->col_type == COLOR_TYPE_DETAIL && objects.detail_being_pulsed[game_object] && col_channel >= 0) {
                col.color = objects.detail_color[game_object];
                if (col_channel == CHANNEL_LIGHTER && objects.main_being_pulsed[game_object]) {
                    col.color = HSV_combine(col.color, lighter_hsv);
                    if (objects.main_col_HSV_enabled[game_object]) {
                        col.color = apply_hsv_to_color(col.color, game_object, true, col_channel);
                    }
                    col.blending = false;
                }
            }

            
            if (obj->col_type == COLOR_TYPE_BLACK) {
                col.color = (Color) {0,0,0};
                
                // Rod base ignore blending
                int obj_id = objects.id[obj->obj];
                if (obj_id >= 15 && obj_id <= 17) {
                    col.blending = false;
                }
            }

            float x = ((objects.x[game_object] - state.camera_x));
            
            float opacity = obj->opacity;

            // Handle invisible object opacity
            if (object_fades(game_object)) {
                float glow_out;
                float fading_opacity = get_fading_obj_fade(game_object, SCREEN_WIDTH / SCALE, &glow_out);
                
                // Check if layer is glow
                if (obj->layer == 1) opacity *= glow_out;
                else opacity *= fading_opacity;
            }

            int real_opacity = get_obj_opacity(game_object, x) * opacity * col.alpha * objects.alpha_trigger_opacity[game_object];

            // Set opacity here
            objects.opacity[game_object] = real_opacity / 255.f;

            obj->blending = col.blending;
            obj->hidden = (real_opacity == 0) || (col.blending && (col.color.r | col.color.g | col.color.b) == 0);
            
            obj->tint = C2D_Color32(col.color.r, col.color.g, col.color.b, real_opacity);
        }
    }
    
    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    snapshot.tint_ms = ticks / CPU_TICKS_PER_MSEC;
}


void create_objects() {
    if (!ensure_render_cache()) return;

    u64 start = svcGetSystemTick();
    render_list_changed = false;

    bool mirror_changed = render_mirror_factor != state.mirror_factor;

    for (int i = 0; i < current_object_count; i++) {
        int obj = current_objects[i];
        int id = objects.id[obj];
        float calc_x = objects.x[obj] - state.camera_x;

        float fade_val = obj_edge_fade(calc_x, SCREEN_WIDTH / SCALE);
        if (fade_val == 255) {
            objects.transition_applied[obj] = FADE_NONE;
        } else if (objects.transition_applied[obj] == FADE_NONE) {
            float calc_y = SCREEN_HEIGHT - (objects.y[obj] - state.camera_y);
            handle_special_fading(obj, calc_x, calc_y);
        }

        if (objects.transition_applied[obj] > FADE_SIMPLE && fade_val != 255) {
            objects.flags[obj] |= FLAG_DIRTY;
        }
        
        // The rotating objects need to be recalculated
        float rotation_speed = get_rotation_speed(obj);
        if (rotation_speed != 0) {
            objects.visual_rotation[obj] += ((objects.random[obj] & 1) ? -rotation_speed : rotation_speed) * delta;
            objects.flags[obj] |= FLAG_DIRTY;
        }

        // Check for pulsing objects, they are dirty
        if (object_has_pulse(id)) objects.flags[obj] |= FLAG_DIRTY;

        // animated objs need to be respawned every frame
        if (game_objects[id].animation_type)
            objects.flags[obj] |= FLAG_DIRTY;

        // Secret coin is animated
        if (id == SECRET_COIN) objects.flags[obj] |= FLAG_DIRTY;

        spawn_object_particles(obj);
    }

    update_current_objects();

    snapshot.draw_count = current_object_count;
    snapshot.draw_dirty = 0;

    if (!mirror_changed && !render_list_changed) {
        // Check if theres dirty objects
        bool has_dirty_objects = false;
        for (int i = 0; i < current_object_count; i++) {
            if (objects.flags[current_objects[i]] & FLAG_DIRTY) {
                has_dirty_objects = true;
                break;
            }
        }

        // No dirty objects, do nothing
        if (!has_dirty_objects && current_object_count > 0) {
            u64 start = svcGetSystemTick();
            render_mirror_factor = state.mirror_factor;
            snapshot.creating_ms = (svcGetSystemTick() - start) / CPU_TICKS_PER_MSEC;
            snapshot.sorting_ms = 0;
            update_tints();
            return;
        }
    }

    // If mirror direction changed, everything is dirty now, jeez
    if (mirror_changed) {
        for (int i = 0; i < current_object_count; i++) {
            objects.flags[current_objects[i]] |= FLAG_DIRTY;
        }
    }

    sprite_count = 0;

    // Player sprite
    // Only needs one as its only for sorting purposes
    memset(&player_sprite_cache, 0, sizeof(player_sprite_cache));
    SpriteObject *vo = &player_sprite_cache;
    vo->obj = -1;
    vo->layer = 0;
    vo->col_type = 0;
    vo->opacity = 1.f;
    vo->col_channel = 0;
    vo->blending = false;
    vo->hidden = false;
    viewable_objects_ptr[sprite_count++] = vo;

    for (int i = 0; i < current_object_count; i++) {
        int obj = current_objects[i];
        int object_start = object_sprite_start[obj];
        unsigned char layer_count = object_sprite_count[obj];

        if (layer_count <= 0) continue;

        if (!(objects.flags[obj] & FLAG_DIRTY)) {
            // Not dirty, avoid recalculating it
            for (int layer = 0; layer < layer_count; layer++) {
                viewable_objects_ptr[sprite_count++] = &object_sprite_cache[object_start + layer];
            }
            continue;
        }

        snapshot.draw_dirty++;

        int visible_start = sprite_count;
        viewable_objects = object_sprite_cache + object_start;
        sprite_count = 0;

        float calc_x = objects.x[obj] - state.camera_x;
        float calc_y = SCREEN_HEIGHT - (objects.y[obj] - state.camera_y);

        float fade_val = obj_edge_fade(calc_x, SCREEN_WIDTH / SCALE);
        
        if (fade_val == 255) {
            objects.transition_applied[obj] = FADE_NONE;
        }

        // Fade when it reaches an edge
        if (fade_val != 255 && objects.transition_applied[obj] == FADE_NONE) {
            handle_special_fading(obj, calc_x, calc_y);
        }
        
        float fade_x = 0;
        float fade_y = 0;
        float fade_scale = 1.f;
        get_fade_vars(obj, calc_x, &fade_x, &fade_y, &fade_scale);

        // Handle special fade types
        fade_x += get_special_fading_vars(obj, fade_val);

        float world_x = get_mirror_x(objects.x[obj], state.mirror_factor);
        float world_y = SCREEN_HEIGHT - objects.y[obj];
        float fade_x_world = (1.f - 2.f * state.mirror_factor) * fade_x;

        spawn_object_at(
            obj, 
            objects.id[obj],        
            world_x + fade_x_world,
            world_y + fade_y, 
            objects.visual_rotation[obj],
            objects.flippedH[obj] ^ (state.mirror_mult < 0),
            objects.flippedV[obj], 
            fade_scale
        );

        layer_count = sprite_count;
        object_sprite_count[obj] = layer_count;

        // Add layers to the list
        for (int layer = 0; layer < layer_count; layer++) {
            viewable_objects_ptr[visible_start + layer] = &object_sprite_cache[object_start + layer];
        }
        sprite_count = visible_start + layer_count;
    
        // Objects in transition are dirty
        objects.flags[obj] &= ~FLAG_DIRTY;
        objects.flags[obj] |= (objects.transition_applied[obj] > FADE_SIMPLE && fade_val != 255) ? FLAG_DIRTY : 0;
    }

    viewable_objects = object_sprite_cache;
    viewable_objects_ptr[0] = &player_sprite_cache;

    render_mirror_factor = state.mirror_factor;
    
    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    snapshot.creating_ms = ticks / CPU_TICKS_PER_MSEC;
    
    start = svcGetSystemTick();
    sort_viewable_objects(viewable_objects_ptr, sprite_count);
    end = svcGetSystemTick();
    ticks = end - start;
    snapshot.sorting_ms = ticks / CPU_TICKS_PER_MSEC;
    
    update_tints();
}

void draw_player_effects() {
    change_blending(true);
    for (int i = 0; i < 2; i++) {
        drawParticleSystem(&drag_particles[i], 0, 0, 1.f);
        drawParticleSystem(&ship_fire_particles[i], 0, 0, 1.f);
        drawParticleSystem(&ship_secondary_particles[i], 0, 0, 1.f);
        drawParticleSystem(&secondary_particles[i], 0, 0, 1.f);
        drawParticleSystem(&burst_particles[i], 0, 0, 1.f);
        drawParticleSystem(&robot_fire_particles[i], 0, 0, 1.f);
        drawParticleSystem(&robot_fire_particles[i], 0, 0, 1.f);
        drawParticleSystem(&land_particles[i], 0, 0, 1.f);
        drawParticleSystem(&explosion_particles[i], 0, 0, 1.f);
    }
    drawParticleSystem(&brick_destroy_particles, 0, 0, 1.f);
    drawParticleSystem(&coin_pickup_particles, 0, 0, 1.f);
    drawParticleSystem(&glitter_particles, 0, 0, 1.f);
    draw_p1_trail(&state.player, 0);
    if (!settingsState.noPlayerTrail) MotionTrail_Draw(&trail_p1);
    MotionTrail_DrawWaveTrail(&wave_trail_p1);
}

void draw_post_player_effects() {
    change_blending(true);
    for (int i = 0; i < 2; i++) {
        drawParticleSystem(&drag_particles_2[i], 0, 0, 1.f);
    }
    change_blending(false);
}

void draw_player_graphics() {
    change_blending(false);
    
    draw_collect_effect();
    draw_key_effect();

    change_blending(true);
    draw_use_effects(get_use_effect_array_ptr(GFX_TOP));
    if (level_info.wall_y > 0) {
        drawParticleSystem(&end_wall_particles, 0, 0, 1);
        // Render rays
        draw_rays();
    }
    draw_object_particles();
    draw_player_effects();

    draw_p1_trail(&state.player2, 1);
    
    if (!settingsState.noPlayerTrail) MotionTrail_Draw(&trail_p2);
    MotionTrail_DrawWaveTrail(&wave_trail_p2);
    change_blending(false);
    state.current_player = 0;
    draw_checkpoints();
    draw_player(&state.player);
    
    if (state.dual) {
        state.current_player = 1;
        draw_player(&state.player2);
    }  

    draw_post_player_effects();
}

void draw_objects() {
    u64 start = svcGetSystemTick();

    C3D_Mtx object_view;
    C2D_ViewSave(&object_view);
    C2D_ViewTranslate((2.f * state.mirror_factor - 1.f) * state.camera_x, state.camera_y);

    // Draw
    for (size_t s = 0; s < sprite_count; s++) {
        SpriteObject *obj = viewable_objects_ptr[s];

        if (obj->obj != -1) {
            if (obj->hidden) continue;

            change_blending(obj->blending);

            if (objects.id[obj->obj] == KEY_OBJ) {
                QuadParams key_params = obj->params;
                float off = key_float_offset(obj->obj);
                key_params.quadr.topLeft[1]  += off;
                key_params.quadr.topRight[1] += off;
                key_params.quadr.botLeft[1]  += off;
                key_params.quadr.botRight[1] += off;
                C2D_DrawImageFast(&obj->spr.image, &key_params, &obj->spr.params, obj->tint);
            } else {
                C2D_DrawImageFast(&obj->spr.image, &obj->params, &obj->spr.params, obj->tint);
            }
        } else {   
            C2D_ViewRestore(&object_view);
            draw_player_graphics();
            C2D_ViewSave(&object_view);
            C2D_ViewTranslate((2.f * state.mirror_factor - 1.f) * state.camera_x, state.camera_y);
        }
    }

    C2D_ViewRestore(&object_view);

    change_blending(true);
    drawParticleSystem(&slow_speed_particles, 0, 0, 1.f);
    drawParticleSystem(&normal_speed_particles, 0, 0, 1.f);
    drawParticleSystem(&fast_speed_particles, 0, 0, 1.f);
    drawParticleSystem(&faster_speed_particles, 0, 0, 1.f);
    change_blending(false);

    if (state.hitbox_display) {
        draw_rotated_hitbox(&state.player);
        draw_player_hitbox(&state.player);
        draw_internal_hitbox(&state.player);
        if (state.hitbox_display == 2) draw_hitbox_trail(0);

        if (state.dual) {
            draw_rotated_hitbox(&state.player2);
            draw_player_hitbox(&state.player2);
            draw_internal_hitbox(&state.player2);
            if (state.hitbox_display == 2) draw_hitbox_trail(1);
        }

        for (int i = 0; i < current_object_count; i++) {
            int obj = current_objects[i];
            draw_hitbox(obj);
        }
    }

    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    snapshot.drawing_ms = ticks / CPU_TICKS_PER_MSEC;
    snapshot.rendering_ms += ticks / CPU_TICKS_PER_MSEC;
}

void update_touch_effect(float delta) {
    touchPosition pos;
    hidTouchRead(&pos);

    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    touch_drag_particles.emitting = false;

    if ((settingsState.touchEffectEverywhere || (game_state == STATE_GAME && !game_paused)) && (kHeld & KEY_TOUCH)) {
        // Flipped for particles
        float flipped_y = SCREEN_HEIGHT - pos.py;

        // Use effect
        if (kDown & KEY_TOUCH) {
            UseEffect *effect = add_use_effect(pos.px, pos.py, USE_EFFECT_OBJ_NOTHING, &tap_effect, get_use_effect_array_ptr(GFX_BOTTOM));    
            touch_explosion_particles.emitterX = pos.px;
            touch_explosion_particles.emitterY = flipped_y;
            spawnMultipleParticles(&touch_explosion_particles, 50);
            if (effect) {
                Color p1_not_white = get_white_if_black(p1_color);

                effect->def.colorR = p1_not_white.r / 255.f;
                effect->def.colorG = p1_not_white.g / 255.f;
                effect->def.colorB = p1_not_white.b / 255.f;
            }
            touch_effect_drag_timer = 0.08f;
        }

        touch_drag_particles.emitterX = pos.px;
        touch_drag_particles.emitterY = flipped_y;
        touch_drag_particles.emitting = (touch_effect_drag_timer <= 0);
        if (touch_effect_drag_timer > 0) {
            touch_effect_drag_timer -= delta;
        }

        
    }
    update_use_effects(delta, get_use_effect_array_ptr(GFX_BOTTOM));
    updateParticleSystem(&touch_explosion_particles, delta);
    updateParticleSystem(&touch_drag_particles, delta);
}

void draw_touch_effect() {
    draw_use_effects(get_use_effect_array_ptr(GFX_BOTTOM));
    drawParticleSystem(&touch_drag_particles, 0, 0, 1.f);
    drawParticleSystem(&touch_explosion_particles, 0, 0, 1.f);
}

void draw_bottom_particles() {
    drawParticleSystem(&glitter_particles_bottom, 0, 0, 1.f);
    drawParticleSystem(&slow_speed_particles_bottom, 0, 0, 1.f);
    drawParticleSystem(&normal_speed_particles_bottom, 0, 0, 1.f);
    drawParticleSystem(&fast_speed_particles_bottom, 0, 0, 1.f);
    drawParticleSystem(&faster_speed_particles_bottom, 0, 0, 1.f);
}

void update_bottom_particles(float delta) {
    glitter_particles_bottom.emitting = false;

    bool flying_gamemode = (state.player.gamemode == GAMEMODE_SHIP || state.player.gamemode == GAMEMODE_UFO || state.player.gamemode == GAMEMODE_WAVE);
    if (state.dual) flying_gamemode = flying_gamemode || (state.player2.gamemode == GAMEMODE_SHIP || state.player2.gamemode == GAMEMODE_UFO || state.player2.gamemode == GAMEMODE_WAVE);

    // If in game and not paused and not fading, update the particles spawning
    if (((game_state == STATE_GAME && !game_paused))) {
        if (flying_gamemode) {
            glitter_particles_bottom.emitterX = state.camera_x_middle;
            glitter_particles_bottom.emitterY = 240/2;
            glitter_particles_bottom.emitting = true;
        }
        slow_speed_particles_bottom.emitting = slow_speed_particles_timer > 0;
        slow_speed_particles_bottom.emitterX = 320/SCALE;
        slow_speed_particles_bottom.emitterY = 240/2;

        normal_speed_particles_bottom.emitting = normal_speed_particles_timer > 0;
        normal_speed_particles_bottom.emitterX = 320/SCALE;
        normal_speed_particles_bottom.emitterY = 240/2;

        fast_speed_particles_bottom.emitting = fast_speed_particles_timer > 0;
        fast_speed_particles_bottom.emitterX = 320/SCALE;
        fast_speed_particles_bottom.emitterY = 240/2;
        
        faster_speed_particles_bottom.emitting = faster_speed_particles_timer > 0;
        faster_speed_particles_bottom.emitterX = 320/SCALE;
        faster_speed_particles_bottom.emitterY = 240/2;
    }

    updateParticleSystem(&glitter_particles_bottom, delta);
    updateParticleSystem(&slow_speed_particles_bottom, delta);
    updateParticleSystem(&normal_speed_particles_bottom, delta);
    updateParticleSystem(&fast_speed_particles_bottom, delta);
    updateParticleSystem(&faster_speed_particles_bottom, delta);
}

C2D_SpriteSheet *get_icon_sheet(const IconPart *part, int gamemode) {
    switch (gamemode) {
        case GAMEMODE_PLAYER:
            return (part->atlas == 0 ? &cube0Sheet : &cube1Sheet);
        case GAMEMODE_SHIP:
            return &shipSheet;
        case GAMEMODE_BALL:
            return &ballSheet;
        case GAMEMODE_UFO:
            return &ufoSheet;
        case GAMEMODE_WAVE:
            return &waveSheet;
        case GAMEMODE_ROBOT:
            return &robotSheet;
    }
    return NULL;
}
/*
static void robot_icon(
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    int robot_anim_id,
    int robot_anim_frame,
    float flip_y_mult, float scale, C2D_ImageTint *tints
) {

    float cos_rot = cosf(C3D_AngleFromDegrees(deg));
    float sin_rot = sinf(C3D_AngleFromDegrees(deg));

    for (int i = 0; i < frame->part_count; i++) {
        const RobotSpritePart *part = &frame->parts[i];

        float part_x = part->px;
        float part_y = part->py * flip_y_mult;

        float rotated_x = (part_x * cos_rot - part_y * sin_rot) * scale;
        float rotated_y = (part_x * sin_rot + part_y * cos_rot) * scale;

        float pos_x = calc_x_mirror + rotated_x * state.mirror_mult;
        float pos_y = calc_y - rotated_y;

        float final_rot = C3D_AngleFromDegrees((part->rotation + player->rotation) * state.mirror_mult);
        float sx = scale * part->scale_x * (flip_x ? -1 : 1);
        float sy = scale * part->scale_y * flip_y_mult;

        for (int layer = 0; layer < 2; layer++) {
            int atlas_idx = (layer == 0) ? robot_l2_atlas[i] : robot_l1_atlas[i];
            u32 tint_color = (layer == 0) ? secondary_color : primary_color;

            C2D_Sprite spr;
            C2D_SpriteFromSheet(&spr, robotSheet, atlas_idx);
            C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
            C2D_SpriteSetPos(&spr, pos_x, pos_y);
            C2D_SpriteSetRotation(&spr, final_rot);
            C2D_SpriteSetScale(&spr, sx, sy);

            C2D_ImageTint tint;
            C2D_PlainImageTint(&tint, tint_color, 1.0f);
            C2D_DrawSpriteTinted(&spr, &tint);
        }
    }
    break;
}*/

static void spawn_icon_at_internal(
    int gamemode,
    int id,
    bool glow,
    bool draw_white,
    float x,
    float y,
    float deg,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale,
    u32 p1_color,
    u32 p2_color,
    u32 glow_color,
    IconParameters params
) {
    const Icon icon = icons[gamemode][id];
    const IconPart *parts = icon.parts;

    float rad = C3D_AngleFromDegrees(deg);
    float cos_r = cosf(rad);
    float sin_r = sinf(rad);

    int flip_x_mult = (flip_x ? -1 : 1);
    int flip_y_mult = (flip_y ? -1 : 1);

    float sx = scale * flip_x_mult;
    float sy = scale * flip_y_mult;

    C2D_Sprite spr = { 0 };

    int count = icon.part_count;

    if (icon.part_count < 2) return;

    C2D_ImageTint tints[4];

    if (draw_white) {
        C2D_PlainImageTint(&tints[ICON_COLOR_WHITE], C2D_Color32(255, 255, 255, 255), 1.0f);
    } else {
        C2D_PlainImageTint(&tints[ICON_COLOR_WHITE], 0, 1.0f);
    }
    C2D_PlainImageTint(&tints[ICON_COLOR_P1], p1_color, 1.0f);
    C2D_PlainImageTint(&tints[ICON_COLOR_P2], p2_color, 1.0f);

    if (glow) {
        C2D_PlainImageTint(&tints[ICON_COLOR_GLOW], glow_color, 1.0f);
    } else {
        C2D_PlainImageTint(&tints[ICON_COLOR_GLOW], 0, 1.0f);
    }

    if (gamemode == GAMEMODE_ROBOT) {
        const RobotAnimation *anim = &robot_animations[params.robot_anim_id];
        if (params.robot_anim_frame >= anim->frame_count)
            params.robot_anim_frame = 0;
        const RobotFrame *frame = &anim->frames[params.robot_anim_frame];
        
        for (int i = 0; i < frame->part_count; i++) {
            const RobotSpritePart *animation_part = &frame->parts[i];

            int texture_part = animation_part->texture_idx / 2;
        
            size_t index;
            for (index = 0; index < count; index++) {
                const IconPart *part = &parts[index];
                if (part->animation_part - 1 == texture_part) {
                    break;
                }
            }
            
            if (index == count) continue;

            // Find layer count
            size_t layer_count = 0;
            while (index + layer_count < count) {
                const IconPart *test_part = &parts[index + layer_count];

                if (test_part->animation_part - 1 != texture_part)
                    break;

                layer_count++;
            }

            for (size_t j = 0; j < layer_count; j++) {
                int real_index = j;

                // Swap p1 and p2
                if (j==0) real_index = 1;
                else if (j==1) real_index = 0;

                const IconPart *part = &parts[index + real_index];

                C2D_SpriteSheet *sheet = get_icon_sheet(part, gamemode);
                if (!sheet) return;

                if (part->texture >= 0) {
                    float part_rad = rad + C3D_AngleFromDegrees(animation_part->rotation);
                    float part_cos_r = cosf(part_rad);
                    float part_sin_r = sinf(part_rad);

                    float anim_x = animation_part->px * 2.0f;
                    float anim_y = animation_part->py * 2.0f;

                    float part_x = part->x;
                    float part_y = part->y;

                    float rot_anim_x = anim_x * cos_r + anim_y * sin_r;
                    float rot_anim_y = anim_x * sin_r - anim_y * cos_r;

                    float rot_part_x = part_x * part_cos_r + part_y * part_sin_r;
                    float rot_part_y = part_x * part_sin_r - part_y * part_cos_r;

                    float p_x = x + (rot_anim_x + rot_part_x) * scale * flip_x_mult;
                    float p_y = y + (rot_anim_y + rot_part_y) * scale * flip_y_mult;

                    C2D_SpriteFromSheet(&spr, *sheet, part->texture);
                    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
                    C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

                    C2D_SpriteSetPos(&spr, p_x, p_y);
                    C2D_SpriteSetScale(&spr, sx, sy);
                    C2D_SpriteSetRotation(&spr, rad + C3D_AngleFromDegrees(animation_part->rotation));

                    C2D_DrawSpriteTinted(&spr, &tints[part->color_type]);
                }
            }
        }
    } else {
        for (size_t i = 0; i < count; i++) {
            size_t real_index = i;
            // Swap p1 and p2 layers
            if (i==0) real_index = 1;
            else if (i==1) real_index = 0;

            if (gamemode == GAMEMODE_UFO) {
                if (i==2) real_index = 0;
                else if (i < 2) real_index++;
            }
            
            const IconPart *part = &parts[real_index];
            C2D_SpriteSheet *sheet = get_icon_sheet(part, gamemode);
            if (!sheet) return;

            if (part->texture >= 0) {

                float local_x = part->x * flip_x_mult;
                float local_y = part->y * flip_y_mult;

                float rot_x = local_x * cos_r + local_y * sin_r;
                float rot_y = local_x * sin_r - local_y * cos_r;

                float p_x = x + rot_x * scale;
                float p_y = y + rot_y * scale;

                C2D_SpriteFromSheet(&spr, *sheet, part->texture);
                C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
                C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

                C2D_SpriteSetPos(&spr, p_x, p_y);
                C2D_SpriteSetScale(&spr, sx, sy);
                C2D_SpriteSetRotation(&spr, rad);

                C2D_DrawSpriteTinted(&spr, &tints[part->color_type]);
            }
        }
    }
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
    u32 glow_color,
    IconParameters params
) {

    if (glow) {
        spawn_glow_layer_at(
            gamemode,
            id,
            x,
            y,
            deg,
            flip_x,
            flip_y,
            scale,
            glow_color,
            params
        );
    }

    spawn_icon_at_internal(
        gamemode,
        id,
        false,
        true,
        x,
        y,
        deg,
        flip_x,
        flip_y,
        scale,
        p1_color,
        p2_color,
        0,
        params
    );
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
    u32 p1_color,
    IconParameters params
) {
    spawn_icon_at_internal(
        gamemode,
        id,
        false,
        false,
        x,
        y,
        deg,
        flip_x,
        flip_y,
        scale,
        p1_color,
        0,
        0,
        params
    );
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
    u32 glow_color,
    IconParameters params
) {
    spawn_icon_at_internal(
        gamemode,
        id,
        true,
        false,
        x,
        y,
        deg,
        flip_x,
        flip_y,
        scale,
        0,
        0,
        glow_color,
        params
    );
}

float approachf(float current, float target, float speed, float smoothing) {
    float diff = target - current;
    float step = diff * smoothing; // smoothing in [0,1], e.g. 0.1 for gentle, 0.5 for fast
    if (fabsf(diff) < speed)
        return target;
    return current + step + (diff > 0 ? speed : -speed);
}


void handle_mirror_transition() {
    if (state.mirroring) {
        // Do the easing
        state.mirror_factor = easeValue(EASE_IN_OUT, state.original_mirror_factor, state.intended_mirror_factor, state.mirror_timer, MIRROR_DURATION, 1.2);

        state.mirror_speed_factor = 1 - 2*state.mirror_factor;
        if (state.mirror_factor >= 0.5f) {
            state.mirror_mult = -1;
        } else {
            state.mirror_mult = 1;
        }
    }
}

