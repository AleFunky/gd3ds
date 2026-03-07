#include "graphics.h"
#include "objects.h"
#include "main.h"
#include "math_helpers.h"
#include "color_channels.h"

bool aaEnabled = false;
bool wideEnabled = false;

int sprite_count = 0;

C2D_SpriteSheet spriteSheet;
C2D_SpriteSheet spriteSheet2;
C2D_SpriteSheet bgSheet;
C2D_SpriteSheet groundSheet;

static SortItem buf_a[MAX_SPRITES];
static SortItem buf_b[MAX_SPRITES];

static SpriteObject viewable_objects[MAX_SPRITES];
static SpriteObject *viewable_objects_ptr[MAX_SPRITES];

int current_fading_effect = FADE_NONE;

static inline float normalize_angle(float a)
{
    while (a < 0.0f)   a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;
    return a;
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

static C2D_SpriteSheet *get_sprite_sheet(int index, int *rel_index) {
	if (index < SPRITESHEET2_START) {
		*rel_index = index;
		return &spriteSheet;
	}

	*rel_index = index - SPRITESHEET2_START;
	return &spriteSheet2;
}

static void spawn_sprite_c2d(
    C2D_Sprite *out,
    C2D_SpriteSheet *sheet,
    int texture,
    float x, float y,
    float scale_x, float scale_y,
    float rotation
) {
    C2D_SpriteFromSheet(out, *sheet, texture);
    C2D_SpriteSetCenter(out, 0.5f, 0.5f);
    C2D_SpriteSetPos(out, (int)x, (int)y);
    C2D_SpriteSetScale(out, scale_x, scale_y);
    C2D_SpriteSetRotation(out, C3D_AngleFromDegrees(rotation));
}

int get_color_channel(int col_type, Object *obj, const GameObject *game_obj) {
	int col_channel = 0;
	if (col_type == COLOR_TYPE_BLACK) col_channel = 0;
	else if (col_type == COLOR_TYPE_WHITE) col_channel = -1;
	else {
		if (obj->v1p9_col_channel) {
			if (col_type == COLOR_TYPE_DETAIL) col_channel = obj->v1p9_col_channel;
		} else {
			if (obj->col_channel) {
				if (col_type == COLOR_TYPE_BASE) {
					col_channel = obj->col_channel;
				} else if (!obj_has_main(game_obj)) {
					col_channel = obj->col_channel;
				}
			}

			if (obj->detail_col_channel) {
				if (col_type == COLOR_TYPE_DETAIL) {
					if (obj_has_main(game_obj)) {
						col_channel = obj->detail_col_channel;
					}
				}
			}
		}
	}
	return col_channel;
}

float get_fading_obj_fade(Object *obj, float x, float right_edge) {
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

void spawn_object_at(
	Object *obj_game,
    int id,
    float x,
    float y,
	float deg,
	unsigned char flip_x,
	unsigned char flip_y,
	float scale
) {
	const GameObject* obj = &game_objects[id];

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

	C2D_Sprite root;

	if (sprite_count >= MAX_SPRITES - 1) return;

	// Skip if no texture
	if (obj->texture >= 0) {
		float local_x = obj->x * flip_x_mult;
		float local_y = obj->y * flip_y_mult;

		float rot_x = local_x * m00 + local_y * m01;
		float rot_y = local_x * m10 + local_y * m11;

		float p_x = x + rot_x * scale;
		float p_y = y + rot_y * scale;

		int texture;
		C2D_SpriteSheet *sheet = get_sprite_sheet(obj->texture, &texture);

		spawn_sprite_c2d(
			&root,
			sheet,
			texture,
			p_x,
			p_y,
			sx, 
			sy,
			deg
		);
		
		SpriteObject *vo = &viewable_objects[sprite_count];

		vo->spr = root;
		vo->obj = obj_game;
		vo->layer = 0;
		vo->col_type = obj->color_type;
		vo->opacity = obj->opacity;
		vo->col_channel = get_color_channel(obj->color_type, obj_game, obj);
		viewable_objects_ptr[sprite_count] = vo;
		sprite_count++;
	}

    // Render children
    for (int i = 0; i < obj->child_count; i++) {
        const ChildSprite* c = &obj->children[i];
		
		if (sprite_count >= MAX_SPRITES - 1) return;
		
		// Skip if no texture
		if (c->texture >= 0) {
			float c_local_x = c->x * flip_x_mult;
			float c_local_y = c->y * flip_y_mult;

			float c_rot_x = c_local_x * m00 + c_local_y * m01;
			float c_rot_y = c_local_x * m10 + c_local_y * m11;

			float c_x = x + c_rot_x * scale;
			float c_y = y + c_rot_y * scale;

            int c_flip_x_mult = (c->flip_x ? -1 : 1);
            int c_flip_y_mult = (c->flip_y ? -1 : 1);
				
			int texture;
			C2D_SpriteSheet *sheet = get_sprite_sheet(c->texture, &texture);

			spawn_sprite_c2d(
				&root,
				sheet,
				texture,
				c_x,
				c_y,
				c->scale_x * c_flip_x_mult * sx,
				c->scale_y * c_flip_y_mult * sy,
				c->rot + deg
			);
				
			SpriteObject *vo = &viewable_objects[sprite_count];

			vo->spr = root;
			vo->obj = obj_game;
			vo->layer = i + 1;
			vo->col_type = c->color_type;
			vo->opacity = c->opacity;
			vo->col_channel = get_color_channel(c->color_type, obj_game, obj);
			viewable_objects_ptr[sprite_count] = vo;
			sprite_count++;
		}
    }
}

static inline uint32_t make_sort_key(const SpriteObject *s)
{
    const Object *obj = s->obj;
    const GameObject *game_obj = &game_objects[obj->id];

    int zlayer = obj->zlayer ? obj->zlayer : game_obj->z_layer;

	// Blending makes zlayer one 
	int col_channel = obj->col_channel;
	if (col_channel > 0 && (channels[col_channel].blending ^ ((zlayer & 1) == 0))) {
		zlayer--;
	}

    int child_z = 0;
	int tex = game_obj->texture;
    if (s->layer > 0) {
        child_z = game_obj->children[s->layer - 1].z;
        tex = game_obj->children[s->layer - 1].texture;
    }

	int sheet = tex < SPRITESHEET2_START ? 1 : 0;

    int zorder = obj->zorder ? obj->zorder : game_obj->z_order;


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

bool object_fades(Object *obj) {
	switch (obj->id) {
		case 146:
		case 147:
		case 206:
		case 204:
		case 673:
		case 674:
		case 144:
		case 145:
		case 205:
		case 459:
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

int get_opacity(Object *obj, float x) {
    int opacity = obj_edge_fade(x, SCREEN_WIDTH / SCALE);

    switch (obj->id) {
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 309:
        case 311:
        case 1747:
        case 1748:
            if (obj->transition_applied == FADE_NONE) opacity = 255;
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
            bool blending = channels[obj->detail_col_channel].blending;
            if (!blending && obj->transition_applied == FADE_NONE) opacity = 255;
            break;
    }

    return opacity;
}

void handle_special_fading(Object *obj, float calc_x, float calc_y) {
    switch (current_fading_effect) {
        case FADE_INWARDS:
            if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                obj->transition_applied = FADE_UP;
            } else {
                obj->transition_applied = FADE_DOWN;
            }
            break;
        case FADE_OUTWARDS:
            if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                obj->transition_applied = FADE_DOWN;
            } else {
                obj->transition_applied = FADE_UP;
            }
            break;
        case FADE_CIRCLE_LEFT:
            if (calc_x > (SCREEN_WIDTH / SCALE / 2)) {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    obj->transition_applied = FADE_UP_STATIONARY;
                } else {
                    obj->transition_applied = FADE_DOWN_STATIONARY;
                }
            } else {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    obj->transition_applied = FADE_UP_SLOW;
                } else {
                    obj->transition_applied = FADE_DOWN_SLOW;
                }
            }
            break;
        case FADE_CIRCLE_RIGHT:
            if (calc_x > (SCREEN_WIDTH / SCALE / 2)) {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    obj->transition_applied = FADE_UP_SLOW;
                } else {
                    obj->transition_applied = FADE_DOWN_SLOW;
                }
            } else {
                if (calc_y > (SCREEN_HEIGHT / SCALE / 2)) {
                    obj->transition_applied = FADE_UP_STATIONARY;
                } else {
                    obj->transition_applied = FADE_DOWN_STATIONARY;
                }
            }
            break;
        default:
            obj->transition_applied = current_fading_effect;  
    }   
}

void get_fade_vars(Object *obj, float x, int *fade_x, int *fade_y, float *fade_scale) {
    switch (obj->transition_applied) {
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
    float calc_y = positive_fmodf(y, offset);

    for (int i = 0; i < 2; i++) {
		C2D_Sprite bg = { 0 };
		// Calculate position for each tile
		float draw_x = -calc_x + i * offset;
		float draw_y = -calc_y;
		
		C2D_SpriteFromSheet(&bg, bgSheet, 0);
		C2D_SpriteSetPos(&bg, (int)draw_x, (int)draw_y);
		C2D_SpriteSetScale(&bg, BACKGROUND_SCALE, BACKGROUND_SCALE);
		C2D_DrawSpriteTinted(&bg, &tint);
	}
}

void draw_ground(float y, bool is_ceiling) {
    int mult = (is_ceiling ? -1 : 1);
	
	C2D_ImageTint tint = { 0 };
	Color col = channels[CHANNEL_GROUND].color;
	C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    // First draw the ground
    float calc_x = 0 - positive_fmodf(cam_x, GROUND_SIZE);
    float calc_y = SCREEN_HEIGHT - ((y - cam_y));

    for (float i = -GROUND_SIZE; i < (SCREEN_WIDTH / SCALE) + GROUND_SIZE; i += GROUND_SIZE) {
		C2D_Sprite ground = { 0 };
		C2D_SpriteFromSheet(&ground, groundSheet, 1);
		C2D_SpriteSetPos(&ground, (int)calc_x + i, (int)calc_y);
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
	C2D_SpriteSetPos(&line, SCREEN_WIDTH / SCALE / 2, (int)((GROUND_SIZE / 2) + calc_y + line_offset));
	C2D_SpriteSetCenter(&line, 0.5f, 0.5f);
	C2D_DrawSpriteTinted(&line, &tint);

    if (channels[CHANNEL_LINE].blending) {
        change_blending(false);
    }
}


void draw_objects() {
	sprite_count = 0;

    int width = (SCREEN_WIDTH / 2) / SECTION_SIZE + 2;
	int cam_sx = (int)((cam_x + SCREEN_WIDTH / 2) / SECTION_SIZE);

    // Create sprites
	for (int x = -width; x <= width; x++) {
		int section = cam_sx + x;
		if (section < 0) continue;

		Section *sec = get_or_create_section(section);
		for (int i = 0; i < sec->object_count; i++) {
			Object *obj = sec->objects[i];
			
            float calc_x = ((obj->x - cam_x));
            float calc_y = SCREEN_HEIGHT - ((obj->y - cam_y));  
			if (calc_x < -60 || calc_x >= (SCREEN_WIDTH / SCALE) + 60) continue;
			if (calc_y < -60 || calc_y >= (SCREEN_HEIGHT / SCALE) + 60) continue;

            int fade_val = obj_edge_fade(calc_x, SCREEN_WIDTH / SCALE);
            bool fade_edge = (fade_val == 255 || fade_val == 0);

			if (fade_edge) handle_special_fading(obj, calc_x, calc_y);
			int fade_x = 0;
			int fade_y = 0;

			float fade_scale = 1.f;

			get_fade_vars(obj, calc_x, &fade_x, &fade_y, &fade_scale);
			
			// Handle special fade types
			if (obj->transition_applied == FADE_DOWN_STATIONARY || obj->transition_applied == FADE_UP_STATIONARY) {
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
				obj->id,
				calc_x,
				calc_y,
				obj->rotation,
				obj->flippedH,
				obj->flippedV,
				fade_scale
			);
		}
	}

    // Sort
    sort_viewable_objects(viewable_objects_ptr, sprite_count);

	int blend_enabled = false;

	// Draw
	C2D_ImageTint tint = { 0 };
	for (size_t s = 0; s < sprite_count; s++) {
		SpriteObject *obj = viewable_objects_ptr[s];
		
		int col_channel = obj->col_channel;

		ColorChannel col;

		if (col_channel < 0) {
			col.color.r = 255;
			col.color.g = 255;
			col.color.b = 255;
			col.blending = false;
		}else {
			col = channels[col_channel];
		}
		
		if (col.blending && !blend_enabled) {
        	change_blending(true);
			blend_enabled = true;
		} else if (!col.blending && blend_enabled) {
        	change_blending(false);
			blend_enabled = false;
		}

		Object *game_object = obj->obj;
		
		float x = ((game_object->x - cam_x));

		int fade_x = 0;
		int fade_y = 0;

		float fade_scale = 1.f;

		get_fade_vars(game_object, x, &fade_x, &fade_y, &fade_scale);

		float opacity = obj->opacity;
		if (object_fades(game_object)) {
			opacity *= get_fading_obj_fade(game_object, x, SCREEN_WIDTH / SCALE);
		}

		C2D_SpriteMove(&obj->spr, fade_x, fade_y);

		// Cull invisible objects
		if ((col.color.r | col.color.g | col.color.b) == 0 && blend_enabled) continue;
		
		C2D_PlainImageTint(&tint, C2D_Color32(col.color.r, col.color.g, col.color.b, get_opacity(game_object, x) * opacity), 1.f);
		C2D_DrawSpriteTinted(&obj->spr, &tint);
	}
}