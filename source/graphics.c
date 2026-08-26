#include "graphics.h"
#include "c2d/base.h"
#include "c2d/spritesheet.h"
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
#include "object_renderer.h"

#include "particles/object_particles.h"
#include "particles/circles.h"
#include "particles/coin_effect.h"

#include "menus/settings.h"
#include "menus/gameplay.h"

#include "menus/core/ui_screen.h"

#include "fonts/bigFont.h"
#include "particles/rays.h"
#include "practice.h"

#include "save/saving.h"
#include "menus/level_select.h"

const Color white = { 255, 255, 255 };

int sprite_count = 0;

static bool blending_state = false;

C2D_SpriteSheet spriteSheet;
C2D_SpriteSheet spriteSheet2;
C2D_SpriteSheet glowSheet;
C2D_SpriteSheet bgSheet;
C2D_SpriteSheet bg2Sheet;
C2D_SpriteSheet groundSheet;
C2D_SpriteSheet iconSheet;
C2D_SpriteSheet trailSheet;
C2D_SpriteSheet particleSheet;

static SortItem buf_a[MAX_SPRITES];
static SortItem buf_b[MAX_SPRITES];

static SpriteObject viewable_objects[MAX_SPRITES];
static SpriteObject *viewable_objects_ptr[MAX_SPRITES];

bool p1_trail = false;
int current_fading_effect = FADE_NONE;
int current_pulserod_ball_image = 0;

SpriteTemplate sprite_templates[GAME_OBJECT_COUNT]; // global cache

float touch_effect_drag_timer = 0.f;

enum ObjectPulseMode {
    OBJECT_PULSE_NONE,
    OBJECT_PULSE_AMPLITUDE,
    OBJECT_PULSE_WIDE,
    OBJECT_PULSE_NARROW,
    OBJECT_PULSE_ROD_CHILD,
};

enum ObjectFadeOpacityMode {
    OBJECT_FADE_NORMAL,
    OBJECT_FADE_ALWAYS_OPAQUE,
    OBJECT_FADE_BASE_IF_OPAQUE,
    OBJECT_FADE_DETAIL_IF_OPAQUE,
};

static float frame_pulse_amplitude = 1.f;
static float frame_pulse_wide = 1.f;
static float frame_pulse_narrow = 1.f;

static float get_rotation_speed_for_id(int id);
static int get_glow_channel_for_id(int id, bool fades);
static u8 get_object_pulse_mode(int id);
static u8 get_fade_opacity_mode_for_id(int id);

static C2D_SpriteSheet *get_sprite_sheet(int index, int *rel_index) {
    // Check if index belongs to spritesheet 1 (most objects)
    if (index < SPRITESHEET2_START) {
        *rel_index = index;
        return &spriteSheet;
    }

    // Return spritesheet 2 (portals)
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

        sprite_templates[id].rotation_speed = get_rotation_speed_for_id(id);
        sprite_templates[id].fades = id == 144 || id == 145 || id == 146 || id == 147
            || id == 204 || id == 205 || id == 206 || id == 459
            || id == 673 || id == 674 || id == 740 || id == 741 || id == 742;
        sprite_templates[id].glow_channel = get_glow_channel_for_id(id, sprite_templates[id].fades);
        sprite_templates[id].pulse_mode = get_object_pulse_mode(id);
        sprite_templates[id].fade_opacity_mode = get_fade_opacity_mode_for_id(id);
        sprite_templates[id].has_particles = object_uses_particles(id);

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
            sprite_templates[id].child_templates = malloc(sizeof(ChildSpriteTemplate) * obj->child_count);
            for (int i = 0; i < obj->child_count; i++) {
                const ChildSprite* c = &obj->children[i];

                float child_rotation = C3D_AngleFromDegrees(c->rot);
                sprite_templates[id].child_templates[i].rotation_sin = sinf(child_rotation);
                sprite_templates[id].child_templates[i].rotation_cos = cosf(child_rotation);

                if (c->texture < 0) continue;

                int c_tex;
                C2D_SpriteSheet *c_sheet = get_sprite_sheet(c->texture, &c_tex);

                C2D_Sprite *child_template = &sprite_templates[id].child_templates[i].sprite;
                C2D_SpriteFromSheet(child_template, *c_sheet, c_tex);
                C3D_TexSetFilter(child_template->image.tex, GPU_LINEAR, GPU_LINEAR);
                C2D_SpriteSetCenter(child_template, 0.5f, 0.5f);
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
    return sprite_templates[objects.id[obj]].fades;
}

inline int get_color_channel(int col_type, int obj, const GameObject *game_obj) {
    int obj_id = objects.id[obj];
    int col_channel = game_obj->base_color;
    if (col_type == COLOR_TYPE_BLACK) col_channel = 0;
    else if (col_type == COLOR_TYPE_WHITE) col_channel = -1;
    else {
        // Check for the presence of 1.9 color channel
        if (objects.v1p9_col_channel[obj]) {
            // If pulserods, use base instead of detail
            if (obj_id >= 15 && obj_id <= 17) {
                if (col_type == COLOR_TYPE_BASE) col_channel = objects.v1p9_col_channel[obj];
            } else {
                if (col_type == COLOR_TYPE_DETAIL) col_channel = objects.v1p9_col_channel[obj];
            }
        } else {
            // 2.0 color channels, here for 1.9 levels that got updated in 2.0 (and for making 1.9 levels in 2.2)
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

// Baby's first reverse engineered function

const float leftFadeBound = (SCREEN_WIDTH_AREA/2) - 75.f;
const float leftFadeWidth = leftFadeBound - 30;
const float rightFadeBound = leftFadeBound + 110;
const float rightFadeWidth = (SCREEN_WIDTH_AREA) - (leftFadeBound + 190);

float get_fading_obj_fade(int obj, float right_edge, float *glow_out) {
    *glow_out = 1.f;

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

// Get the glow color channel. This depends only on the object definition, so
// cache it with the sprite template instead of switching for every instance.
static int get_glow_channel_for_id(int id, bool fades) {
    if (fades) {
        return CHANNEL_INVISIBLE_GLOW;
    }

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
        case 35:
        case 36:
            return CHANNEL_YELLOW_GLOW;
        case 67:
        case 84:
            return CHANNEL_BLUE_GLOW;
        case 140:
        case 141:
            return CHANNEL_PINK_GLOW;
        case 200:
        case 201:
        case 202:
        case 203:
            return CHANNEL_WHITE;
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

int get_glow_channel(int obj) {
    return sprite_templates[objects.id[obj]].glow_channel;
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
            return get_coin_texture(tex + (is_coin_collected(obj) ? 12 : 0), 26);
    }
    return -1;
}

// Deco saws rotate slower than normal saws. If not a saw, rotation speed is just 0
static float get_rotation_speed_for_id(int id) {
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

static u8 get_object_pulse_mode(int id) {
    switch (id) {
        case 36:
        case 84:
        case 141:
            return OBJECT_PULSE_WIDE;
        case 15:
        case 16:
        case 17:
            return OBJECT_PULSE_ROD_CHILD;
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 60:
        case 148:
        case 149:
        case 405:
            return OBJECT_PULSE_AMPLITUDE;
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
            return OBJECT_PULSE_NARROW;
    }
    return OBJECT_PULSE_NONE;
}

static inline float get_cached_object_pulse(int id, int layer) {
    switch (sprite_templates[id].pulse_mode) {
        case OBJECT_PULSE_AMPLITUDE:
            return frame_pulse_amplitude;
        case OBJECT_PULSE_WIDE:
            return frame_pulse_wide;
        case OBJECT_PULSE_NARROW:
            return frame_pulse_narrow;
        case OBJECT_PULSE_ROD_CHILD:
            return layer == 2 ? frame_pulse_amplitude : 1.f;
        default:
            return 1.f;
    }
}

static u8 get_fade_opacity_mode_for_id(int id) {
    switch (id) {
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
            return OBJECT_FADE_ALWAYS_OPAQUE;
        case 211:
            return OBJECT_FADE_BASE_IF_OPAQUE;
        case 207:
        case 208:
        case 209:
        case 210:
        case 212:
        case 213:
        case 331:
        case 333:
        case 693:
        case 694:
            return OBJECT_FADE_DETAIL_IF_OPAQUE;
        default:
            return OBJECT_FADE_NORMAL;
    }
}

// Keep the compact per-object transform in the render list. The raw Citro3D
// object renderer expands it in its vertex shader instead of rotating all four
// corners on the CPU through Citro2D.
static inline void set_sprite_render_data(
    SpriteObject *sprite,
    const C2D_Image image,
    float x,
    float y,
    float scale_x,
    float scale_y,
    float rotation_sin,
    float rotation_cos
) {
    sprite->image = image;
    sprite->x = x;
    sprite->y = y;
    sprite->half_width = fabsf(scale_x * image.subtex->width) * 0.5f;
    sprite->half_height = fabsf(scale_y * image.subtex->height) * 0.5f;
    sprite->rotation_sin = rotation_sin;
    sprite->rotation_cos = rotation_cos;
    sprite->flip_x = scale_x < 0.f;
    sprite->flip_y = scale_y < 0.f;
}

static inline void prepare_sprite_color(SpriteObject *sprite, int edge_opacity, float fade_opacity) {
    int col_channel = sprite->col_channel;
    ColorChannel col;
    int channel_index;
    bool mix_invisible_glow = false;

    if (col_channel < 0) {
        channel_index = COL_CHANNEL_NUM;
        col.color.r = 255;
        col.color.g = 255;
        col.color.b = 255;
        col.blending = false;
    } else if (col_channel == CHANNEL_INVISIBLE_GLOW) {
        channel_index = get_col_channel_index(CHANNEL_LBG_NOLERP);
        col = channels[channel_index];
        float opacity = objects.opacity[sprite->obj];

        if (opacity >= 0.8f && !state.dead) {
            mix_invisible_glow = true;
            col.blending = true;
        }
    } else {
        channel_index = get_col_channel_index(col_channel);
        col = channels[channel_index];
    }

    int real_opacity = edge_opacity * sprite->opacity * fade_opacity;
    int parent_opacity = mix_invisible_glow
        ? CLAMP((int)(objects.opacity[sprite->obj] * 255.f), 0, 255)
        : 0;
    sprite->color_meta = (u32)channel_index
        | ((u32)(real_opacity & 0xff) << 8)
        | ((u32)parent_opacity << 16)
        | ((u32)mix_invisible_glow << 24);
    sprite->blending = col.blending;
    sprite->visible = real_opacity > 0
        && (mix_invisible_glow || !col.blending || (col.color.r | col.color.g | col.color.b) != 0);
}

void spawn_object_at(
    int obj_game,
    int id,
    float x,
    float y,
    float sin_r,
    float cos_r,
    unsigned char flip_x,
    unsigned char flip_y,
    float scale,
    int edge_opacity,
    float fading_opacity,
    float glow_opacity
) {
    const GameObject* obj = &game_objects[id];

    // A vertical mirror negates the angle. Its cosine is unchanged.
    if (state.mirror_mult < 0) sin_r = -sin_r;

    int flip_x_mult = (flip_x ? -1 : 1);
    int flip_y_mult = (flip_y ? -1 : 1);

    float m00 = cos_r;
    float m01 = sin_r;
    float m10 = sin_r;
    float m11 = -cos_r;

    float sx = scale * flip_x_mult;
    float sy = scale * flip_y_mult;

    if (sprite_count >= MAX_SPRITES - 1) return;

    // Invisible glow channels depend on the parent opacity. Resolve it before
    // spawning any layer so glow ordering cannot make it use the prior frame.
    if (obj->texture >= 0) {
        objects.opacity[obj_game] = (edge_opacity * obj->opacity * fading_opacity) / 255.f;
    }

    // Spawn parent, skip if no texture
    if (obj->texture >= 0) {
        SpriteObject *vo = &viewable_objects[sprite_count];

        float local_x = obj->x * flip_x_mult;
        float local_y = obj->y * flip_y_mult;

        float rot_x = local_x * m00 + local_y * m01;
        float rot_y = local_x * m10 + local_y * m11;

        float p_x = x + rot_x * scale;
        float p_y = y + rot_y * scale;

        int random_layer = get_obj_random_layer(obj_game, id);
        C2D_Image image;
        if (random_layer < 0) {
            image = sprite_templates[id].parent_template.image;
        } else {
            int rel_index;
            C2D_SpriteSheet *sheet = get_sprite_sheet(random_layer, &rel_index);
            image = C2D_SpriteSheetGetImage(*sheet, rel_index);
        }

        float pulse_scale = get_cached_object_pulse(id, 0);

        set_sprite_render_data(vo, image, p_x, p_y, sx * pulse_scale, sy * pulse_scale, sin_r, cos_r);

        vo->obj = obj_game;
        vo->layer = 0;
        vo->opacity = obj->opacity;
        vo->col_channel = get_color_channel(obj->color_type, obj_game, obj);
        prepare_sprite_color(vo, edge_opacity, fading_opacity);
        viewable_objects_ptr[sprite_count] = vo;
        sprite_count++;
    }

    // Skip if no glow frame
    if (settingsState.glowEnabled && obj->glow_frame >= 0) {
        if (sprite_count >= MAX_SPRITES - 1) return;

        SpriteObject *vo = &viewable_objects[sprite_count];

        float pulse_scale = get_cached_object_pulse(id, 1);

        set_sprite_render_data(vo, sprite_templates[id].glow_template.image,
            x, y, sx * pulse_scale, sy * pulse_scale, sin_r, cos_r);

        vo->obj = obj_game;
        vo->layer = 1;
        vo->opacity = obj->opacity;
        vo->col_channel = get_glow_channel(obj_game);
        prepare_sprite_color(vo, edge_opacity, glow_opacity);
        viewable_objects_ptr[sprite_count] = vo;
        sprite_count++;
    }

    // Spawn children
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

            float pulse_scale = get_cached_object_pulse(id, i + 2);
            const ChildSpriteTemplate *child_template = &sprite_templates[id].child_templates[i];
            if (id < 15 || id > 17) {
                float child_sin = sin_r * child_template->rotation_cos + cos_r * child_template->rotation_sin;
                float child_cos = cos_r * child_template->rotation_cos - sin_r * child_template->rotation_sin;
                set_sprite_render_data(vo, child_template->sprite.image, c_x, c_y,
                    c->scale_x * c_flip_x_mult * sx * pulse_scale,
                    c->scale_y * c_flip_y_mult * sy * pulse_scale,
                    child_sin, child_cos);
            } else {
                set_sprite_render_data(vo, child_template->sprite.image, c_x, c_y,
                    fabsf(c->scale_x * c_flip_x_mult * sx * pulse_scale),
                    fabsf(c->scale_y * c_flip_y_mult * sy * pulse_scale),
                    0.f, 1.f);
            }

            vo->obj = obj_game;
            vo->layer = i + 2;
            vo->opacity = c->opacity;
            vo->col_channel = get_color_channel(c->color_type, obj_game, obj);
            prepare_sprite_color(vo, edge_opacity, fading_opacity);
            viewable_objects_ptr[sprite_count] = vo;
            sprite_count++;
        }
    }
}

static inline uint32_t make_sort_key(SpriteObject *s)
{
    const int obj = s->obj;

    // Player sprite is -1 so handle it there
    if (obj == -1) {
        return (5 + 8) << 19;
    }

    const int id = objects.id[obj];
    const GameObject *game_obj = &game_objects[id];

    int zlayer = objects.zlayer[obj] ? objects.zlayer[obj] : game_obj->z_layer;

    // Blending makes zlayer one 
    int col_channel = s->col_channel;

    bool blending = col_channel > 0 && (channels[get_col_channel_index(col_channel)].blending ^ ((zlayer & 1) == 0));
    bool inset_speed_glow = s->layer == 1 && id >= SLOW_SPEED_PORTAL && id <= FASTER_SPEED_PORTAL;

    // A speed portal is intentionally sandwiched between the back and front
    // halves of the other portals. Keep its glow in that same layer, directly
    // behind its arrow, instead of applying the generic full-layer glow drop.
    if (inset_speed_glow) blending = false;

    // If layer is a glow layer or it has blending, decrement it
    if ((s->layer == 1 && !inset_speed_glow) || blending) {
        zlayer--;
    }

    int child_z = inset_speed_glow ? -1 : 0;
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
        sheet = inset_speed_glow ? 0 : 2;
    } else {
        sheet = tex < SPRITESHEET2_START ? 1 : 0;
    }
    
    int zorder = objects.zorder[obj] ? objects.zorder[obj] : game_obj->z_order;

    // Move the pulserod ball
    if (id >= 15 && id <= 17 && s->layer == 2) {
        zlayer += 2;
    } 

    // Combine blending and sheet order into one real three-bit field. Glow
    // uses sheet value 2, so the old one-bit packing collided with blending
    // and could put otherwise identical layers on the wrong side of each
    // other. Five z-layer bits retain the three-pass radix sort.
    uint32_t zl = (uint32_t)CLAMP(zlayer + 8, 0, 31);
    uint32_t material = (uint32_t)blending * 3 + (uint32_t)sheet;
    uint32_t zo = (uint32_t)(zorder + 128);   // fits in 8 bits
    uint32_t cz = (uint32_t)(child_z + 128);  // fits in 8 bits

    return (zl << 19) | (material << 16) | (zo << 8) | cz;
}

#define VIEW_OBJECTS (12 * 6)
#define INSERTION_SORT_THRESHOLD 16

// Insertion sort moment
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

// Some objects dont change opacity on fade transitions.
static int get_obj_opacity_from_fade(int obj, int opacity) {
    if (objects.transition_applied[obj] != FADE_NONE) return opacity;

    switch (sprite_templates[objects.id[obj]].fade_opacity_mode) {
        case OBJECT_FADE_ALWAYS_OPAQUE:
            return 255;
        case OBJECT_FADE_BASE_IF_OPAQUE:
            if (!channels[get_col_channel_index(objects.col_channel[obj])].blending) return 255;
            break;
        case OBJECT_FADE_DETAIL_IF_OPAQUE:
            if (!channels[get_col_channel_index(objects.detail_col_channel[obj])].blending) return 255;
            break;
    }

    return opacity;
}

int get_obj_opacity(int obj, float x) {
    return get_obj_opacity_from_fade(obj, obj_edge_fade(x, SCREEN_WIDTH / SCALE));
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

static void get_fade_vars_from_value(int obj, int fade, int *fade_x, int *fade_y, float *fade_scale) {
    int offset = (255 - fade) / 2;

    switch (objects.transition_applied[obj]) {
        case FADE_NONE:
            break;
        case FADE_UP:
            *fade_y = offset;
            break;
        case FADE_DOWN:
            *fade_y = -offset;
            break;
        case FADE_RIGHT:
            *fade_x = offset;
            break;
        case FADE_LEFT:
            *fade_x = -offset;
            break;
        case FADE_SCALE_IN:
            *fade_scale = fade / 255.f;
            break;
        case FADE_SCALE_OUT:
            *fade_scale = 1.f + ((255 - fade) / 255.f) / 2.f;
            break;
        case FADE_UP_SLOW_LEFT:
            *fade_x = -offset;
            *fade_y = offset / 3;
            break;
        case FADE_UP_SLOW_RIGHT:
            *fade_x = offset;
            *fade_y = offset / 3;
            break;
        case FADE_UP_STATIONARY:
            *fade_y = offset / 3;
            break;
        case FADE_DOWN_SLOW_LEFT:
            *fade_x = -offset;
            *fade_y = -offset / 3;
            break;
        case FADE_DOWN_SLOW_RIGHT:
            *fade_x = offset;
            *fade_y = -offset / 3;
            break;
        case FADE_DOWN_STATIONARY:
            *fade_y = -offset / 3;
            break;
    }
}

void get_fade_vars(int obj, float x, int *fade_x, int *fade_y, float *fade_scale) {
    get_fade_vars_from_value(obj, obj_edge_fade(x, SCREEN_WIDTH / SCALE), fade_x, fade_y, fade_scale);
}

void get_special_fading_vars(int obj, float fade_val, float *calc_x) {
    if (objects.transition_applied[obj] == FADE_DOWN_STATIONARY || objects.transition_applied[obj] == FADE_UP_STATIONARY) {
        if (fade_val < 255) {
            if (*calc_x > (SCREEN_WIDTH / SCALE) / 2) {
                *calc_x = SCREEN_WIDTH / SCALE - FADE_WIDTH;
            } else {
                *calc_x = FADE_WIDTH;
            }
        }
    }
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

    for (int i = -1; i < 3; i++) {
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
    Color col = channels[get_col_channel_index(CHANNEL_GROUND)].color;
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
        draw_text(&bigFont_fontCharset, &bigFont_sheet, get_mirror_x(calc_x, state.mirror_factor), calc_y, 1, (settingsState.doNot ? -1 : 1), 0.5f, true, "Attempt %d", attempts);
    }
}

float object_creating_time = 0;
float object_sorting_time = 0;
float object_drawing_time = 0;


void create_objects() {
    sprite_count = 0;

    frame_pulse_amplitude = amplitude;
    frame_pulse_amplitude *= music_volume > 0 && global_volume > 0;
    frame_pulse_amplitude = MAX(0.1f, frame_pulse_amplitude);
    frame_pulse_wide = 0.3f + frame_pulse_amplitude * 0.9f;
    frame_pulse_narrow = 0.6f + frame_pulse_amplitude * 0.6f;

    // Saw speeds are limited to 180 or 360 degrees per second. Compute both
    // frame steps once, then advance each visible saw with angle addition.
    float step_180 = C3D_AngleFromDegrees(180.f * delta);
    float step_360 = C3D_AngleFromDegrees(360.f * delta);
    float step_180_sin = sinf(step_180);
    float step_180_cos = cosf(step_180);
    float step_360_sin = sinf(step_360);
    float step_360_cos = cosf(step_360);

    // Player sprite
    // Only needs one as its only for sorting purposes
    SpriteObject *vo = &viewable_objects[sprite_count];

    vo->obj = -1;
    vo->layer = 0;
    vo->opacity = 1.f;
    vo->col_channel = 0;
    vo->blending = false;
    vo->visible = true;
    viewable_objects_ptr[sprite_count] = vo;
    sprite_count++;

    int width = ceilf((SCREEN_WIDTH_AREA) / SECTION_SIZE);
    int height = ceilf((SCREEN_HEIGHT_AREA) / SECTION_SIZE);
    int cam_sx = (int)((state.camera_x) / SECTION_SIZE);
    int cam_sy = (int)((state.camera_y - LEVEL_Y_OFFSET) / SECTION_SIZE);
    u64 start = svcGetSystemTick();
    // Create sprites
    for (int x = -1; x <= width; x++) {
        for (int y = -1; y <= height; y++) {
            int sx = cam_sx + x;
            int sy = cam_sy + y;
            if (sx < 0) continue;
            if (sy < 0) continue;

            Section *sec = get_section(sx, sy);
            for (int i = 0; i < sec->object_count; i++) {
                int obj = sec->objects[i];
                
                float calc_x = (objects.x[obj] - state.camera_x);
                float calc_y = SCREEN_HEIGHT - ((objects.y[obj] - state.camera_y));  
                if (calc_x < -60 || calc_x >= (SCREEN_WIDTH / SCALE) + 60) continue;
                if (calc_y < -60 || calc_y >= (SCREEN_HEIGHT / SCALE) + 60) continue;

                // Skip invalid objects
                if (!is_valid_object(objects.id[obj]) || objects.toggled[obj]) {
                    continue;
                }

                int fade_val = obj_edge_fade(calc_x, SCREEN_WIDTH / SCALE);
                bool fade_edge = (fade_val == 255 || fade_val == 0);

                if (fade_edge) {
                    if (current_fading_effect == FADE_NONE) {
                        objects.transition_applied[obj] = FADE_NONE;
                    } else {
                        handle_special_fading(obj, calc_x, calc_y);
                    }
                }
                int fade_x = 0;
                int fade_y = 0;

                float fade_scale = 1.f;

                u8 fade_transition = objects.transition_applied[obj];
                if (fade_transition != FADE_NONE) {
                    get_fade_vars_from_value(obj, fade_val, &fade_x, &fade_y, &fade_scale);
                }

                // Handle saw rotation
                float rotation_sin = objects.rotation_sin[obj];
                float rotation_cos = objects.rotation_cos[obj];
                float rotation_speed = sprite_templates[objects.id[obj]].rotation_speed;
                if (rotation_speed != 0.f) {
                    bool reverse = (objects.random[obj] & 1) != 0;
                    float step_sin = rotation_speed == 360.f ? step_360_sin : step_180_sin;
                    float step_cos = rotation_speed == 360.f ? step_360_cos : step_180_cos;
                    if (reverse) {
                        rotation_speed = -rotation_speed;
                        step_sin = -step_sin;
                    }

                    objects.rotation[obj] += rotation_speed * delta;
                    float old_sin = rotation_sin;
                    rotation_sin = old_sin * step_cos + rotation_cos * step_sin;
                    rotation_cos = rotation_cos * step_cos - old_sin * step_sin;

                    // Bound floating-point drift without returning to per-object
                    // trigonometry on every frame.
                    if ((level_frame & 0x1ff) == 0) {
                        float radians = C3D_AngleFromDegrees(objects.rotation[obj]);
                        rotation_sin = sinf(radians);
                        rotation_cos = cosf(radians);
                    }

                    objects.rotation_sin[obj] = rotation_sin;
                    objects.rotation_cos[obj] = rotation_cos;
                }
                
                // Handle special fade types
                if (fade_transition == FADE_DOWN_STATIONARY || fade_transition == FADE_UP_STATIONARY) {
                    get_special_fading_vars(obj, fade_val, &calc_x);
                }

                int edge_opacity = get_obj_opacity_from_fade(obj, fade_val);
                float fading_opacity = 1.f;
                float glow_opacity = 1.f;
                if (object_fades(obj)) {
                    fading_opacity = get_fading_obj_fade(obj, SCREEN_WIDTH / SCALE, &glow_opacity);
                }

                spawn_object_at(
                    obj,
                    objects.id[obj],
                    get_mirror_x(calc_x + fade_x, state.mirror_factor),
                    calc_y + fade_y,
                    rotation_sin,
                    rotation_cos,
                    objects.flippedH[obj] ^ (state.mirror_mult < 0),
                    objects.flippedV[obj],
                    fade_scale,
                    edge_opacity,
                    fading_opacity,
                    glow_opacity
                );

                if (sprite_templates[objects.id[obj]].has_particles) {
                    spawn_object_particles(obj);
                }
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
}

void draw_player_effects() {
    change_blending(true);
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

    change_blending(true);
    draw_use_effects(get_use_effect_array_ptr(GFX_TOP));
    if (level_info.wall_y > 0) {
        drawParticleSystem(&end_wall_particles, 0, 0, 1);
        // Render rays
        draw_rays(delta);
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

static void draw_object_range(size_t begin, size_t end) {
    bool renderer_active = false;
    C3D_Tex *batch_texture = NULL;
    bool batch_blending = false;
    int batch_first_slot = 0;
    int batch_sprite_count = 0;

    for (size_t s = begin; s < end; s++) {
        SpriteObject *obj = viewable_objects_ptr[s];
        if (obj->obj == -1 || !obj->visible) continue;

        bool continues_batch = batch_sprite_count > 0
            && obj->image.tex == batch_texture
            && obj->blending == batch_blending
            && obj->render_slot == batch_first_slot + batch_sprite_count;

        if (!continues_batch && batch_sprite_count > 0) {
            object_renderer_draw_batch(batch_first_slot, batch_sprite_count, batch_texture, batch_blending);
            batch_sprite_count = 0;
        }

        if (batch_sprite_count == 0) {
            if (!renderer_active) {
                object_renderer_begin();
                renderer_active = true;
            }
            batch_texture = obj->image.tex;
            batch_blending = obj->blending;
            batch_first_slot = obj->render_slot;
        }
        batch_sprite_count++;
    }

    if (batch_sprite_count > 0) {
        object_renderer_draw_batch(batch_first_slot, batch_sprite_count, batch_texture, batch_blending);
    }
    if (renderer_active) {
        object_renderer_end();
        blending_state = false;
    }
}

void draw_objects() {
    u64 start = svcGetSystemTick();

    // The left eye is always first. Build the GPU buffer once after FrameBegin
    // has synchronized with the previous frame, then reuse it for the right eye.
    if (!is_extra_eye()) {
        object_renderer_build(viewable_objects_ptr, sprite_count);
    }

    size_t player_pos = 0;
    while (player_pos < sprite_count && viewable_objects_ptr[player_pos]->obj != -1) player_pos++;

    draw_object_range(0, player_pos);
    if (player_pos < sprite_count) {
        draw_player_graphics();
        draw_object_range(player_pos + 1, sprite_count);
    }

    change_blending(true);
    drawParticleSystem(&slow_speed_particles, 0, 0, 1.f);
    drawParticleSystem(&normal_speed_particles, 0, 0, 1.f);
    drawParticleSystem(&fast_speed_particles, 0, 0, 1.f);
    drawParticleSystem(&faster_speed_particles, 0, 0, 1.f);
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

    if ((settingsState.touchEffectEverywhere || (game_state == STATE_GAME && !game_paused)) && (kHeld & KEY_TOUCH) && !get_fade_status()) {
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

    bool flying_gamemode = (state.player.gamemode == GAMEMODE_SHIP || state.player.gamemode == GAMEMODE_BIRD || state.player.gamemode == GAMEMODE_DART);
    if (state.dual) flying_gamemode = flying_gamemode || (state.player2.gamemode == GAMEMODE_SHIP || state.player2.gamemode == GAMEMODE_BIRD || state.player2.gamemode == GAMEMODE_DART);

    // If in game and not paused and not fading, update the particles spawning
    if (((game_state == STATE_GAME && !game_paused)) && !get_fade_status()) {
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
