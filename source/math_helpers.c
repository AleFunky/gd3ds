#include <math.h>
#include "math_helpers.h"
#include "color_channels.h"

float clampf(float d, float min, float max) {
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

float positive_fmodf(float n, float divisor) {
    float value = fmodf(n, divisor);
    return value + (value < 0 ? divisor : 0);
}

Color color_lerp(Color color1, Color color2, float fraction) {
    unsigned char r1 = color1.r;
    unsigned char r2 = color2.r;
    unsigned char g1 = color1.g;
    unsigned char g2 = color2.g;
    unsigned char b1 = color1.b;
    unsigned char b2 = color2.b;

    Color returned;
    returned.r = (r2 - r1) * fraction + r1;
    returned.g = (g2 - g1) * fraction + g1;
    returned.b = (b2 - b1) * fraction + b1;

    return returned;
}