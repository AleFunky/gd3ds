#pragma once
#include "color_channels.h"

float clampf(float d, float min, float max);
float positive_fmodf(float n, float divisor);
Color color_lerp(Color color1, Color color2, float fraction);