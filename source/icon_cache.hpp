#pragma once

#include <citro2d.h>

void icon_cache_init(void);
void icon_cache_shutdown(void);
void icon_cache_begin_frame(void);
C2D_SpriteSheet icon_cache_get(int gamemode, int id);
