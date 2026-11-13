#pragma once

#include <citro2d.h>

namespace gd3ds
{

void iconCacheInit(void);
void iconCacheShutdown(void);
void iconCacheBeginFrame(void);
C2D_SpriteSheet iconCacheGet(int gamemode, int id);

}
