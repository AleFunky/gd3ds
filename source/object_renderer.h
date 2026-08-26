#pragma once

#include <citro3d.h>

typedef struct SpriteObject SpriteObject;

bool object_renderer_init(void);
void object_renderer_fini(void);

void object_renderer_build(SpriteObject *const *objects, int count);
void object_renderer_begin(void);
void object_renderer_draw_batch(int first_slot, int sprite_count, C3D_Tex *texture, bool blending);
void object_renderer_end(void);
