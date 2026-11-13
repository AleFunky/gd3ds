#include <math.h>
#include "math_helpers.hpp"
#include "triggers.hpp"

#include <stdlib.h>
#include <float.h>

namespace gd3ds
{

Matrix2x2 matrix2x2Identity(void) {
    return (Matrix2x2){ { { 1.f, 0.f }, { 0.f, 1.f } } };
}

Matrix2x2 matrix2x2Rotation(float radians) {
    float cosine = cosf(radians);
    float sine = sinf(radians);
    return (Matrix2x2){ { { cosine, -sine }, { sine, cosine } } };
}

Matrix2x2 matrix2x2Multiply(Matrix2x2 a, Matrix2x2 b) {
    Matrix2x2 result;
    result.m[0][0] = a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0];
    result.m[0][1] = a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1];
    result.m[1][0] = a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0];
    result.m[1][1] = a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1];
    return result;
}

Vec2D matrix2x2Transform(Matrix2x2 matrix, Vec2D vector) {
    return (Vec2D){
        matrix.m[0][0] * vector.x + matrix.m[0][1] * vector.y,
        matrix.m[1][0] * vector.x + matrix.m[1][1] * vector.y
    };
}

Vec2D affineTransformPoint(Matrix2x2 matrix, Vec2D offset, Vec2D point) {
    return addVec(matrix2x2Transform(matrix, point), offset);
}

void composeAffineTransform(Matrix2x2 *currentMatrix, Vec2D *currentOffset, Matrix2x2 nextMatrix, Vec2D nextOffset) {
    *currentOffset = addVec(matrix2x2Transform(nextMatrix, *currentOffset), nextOffset);
    *currentMatrix = matrix2x2Multiply(nextMatrix, *currentMatrix);
}

Vec2D addVec(Vec2D a, Vec2D b) {
    return (Vec2D) { a.x + b.x, a.y + b.y};
}

Vec2D subVec(Vec2D a, Vec2D b) {
    return (Vec2D) { a.x - b.x, a.y - b.y};
}

Vec2D multiplyVec(Vec2D v, float val) {
    return (Vec2D) { v.x * val, v.y * val};
}

float lenVec(Vec2D v) {
    return sqrtf(v.x*v.x + v.y*v.y);
}

Vec2D normalize(Vec2D v) {
    float len = lenVec(v);
    if (len < 0.0001f) len = 1.f;
    return (Vec2D){ v.x / len, v.y / len };
}

float dotVec(Vec2D a, Vec2D b) {
    return a.x * b.x + a.y * b.y;
}

Vec2D perpendicular(Vec2D v) {
    return (Vec2D){ -v.y, v.x };
}

float squareDistVec(const Vec2D* a, const Vec2D* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    return dx*dx + dy*dy;
}

float clampf(float d, float min, float max) {
    const float t = d < min ? min : d;
    return t > max ? max : t;
}

float repeat(float a, float length) {
	return clampf(a - floorf(a / length) * length, 0.0f, length);
}

float positiveFmodf(float n, float divisor) {
    float value = fmodf(n, divisor);
    return value + (value < 0 ? divisor : 0);
}

Color colorLerp(Color color1, Color color2, float fraction) {
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
u32 colorLerpU32(u32 color1, u32 color2, float fraction) {
    unsigned char r1 = GET_R(color1);
    unsigned char r2 = GET_R(color2);
    unsigned char g1 = GET_G(color1);
    unsigned char g2 = GET_G(color2);
    unsigned char b1 = GET_B(color1);
    unsigned char b2 = GET_B(color2);

    unsigned char r = (r2 - r1) * fraction + r1;
    unsigned char g = (g2 - g1) * fraction + g1;
    unsigned char b = (b2 - b1) * fraction + b1;

    return RGB8(r, g, b);
}

// Generic easing out function, not from GD
float easeOut(float current, float target, float smoothing) {
    return current + (target - current) * smoothing;
}

float mapRange(float val, float min1, float max1, float min2, float max2) {
    return min2 + ((max2 - min2) / (max1 - min1)) * (val - min1);
}

float slerp(float a, float b, float ratio) {
	float delta = repeat((b - a), 360.f);
	if (delta > 180.f)
		delta -= 360.f;
	return a + delta * clampf(ratio, 0.f, 1.f);
}

// GD's slerp
float slerpFancy(float fromAngle, float toAngle, float t) {
    float fromHalf = fromAngle * 0.5f;
    float toHalf   = toAngle   * 0.5f;

    float fx = cosf(fromHalf);
    float fy = sinf(fromHalf);
    float tx = cosf(toHalf);
    float ty = sinf(toHalf);

    float dot = fx * tx + fy * ty;

    if (dot < 0.0f) {
        dot = -dot;
        tx = -tx;
        ty = -ty;
    }

    float w0 = 1.0f - t;
    float w1 = t;

    if (dot < 0.9999f) {
        float between = acosf(dot);
        float sinBetween = sinf(between);

        w0 = sinf(w0 * between) / sinBetween;
        w1 = sinf(w1 * between) / sinBetween;
    }

    float ix = w0 * fx + w1 * tx;
    float iy = w0 * fy + w1 * ty;

    return atan2f(iy, ix) * 2.0f;
}

float lerp(float from, float to, float alpha) {
    return from * (1.0f - alpha) + to * alpha;
}

float iLerp(float a, float b, float ratio, float dt) {
	const float rDelta = dt * STEPS_HZ;
	const float s	  = 1.f - ratio;

	float iRatio = 1.f - powf(s, rDelta);

	return lerp(a, b, iRatio);
}

float iSlerp(float a, float b, float ratio, float dt) {
	const float rDelta = dt * STEPS_HZ;
	const float s	  = 1.f - ratio;

	float iRatio = 1.f - powf(s, rDelta);

	return slerp(a, b, iRatio);
}

bool isEffectivelyInteger(float x) {
    return fabsf(x - roundf(x)) < FLT_EPSILON;
}

float ip1Ceilf(float x) {
    float c = ceilf(x);
    if (isEffectivelyInteger(x)) {
        return c + 1.0f;
    }
    return c;
}


float adjustAngle(float angle, int flipX, int flipY) {
    // Normalize to [0, 360)
    angle = positiveFmodf(angle, 360);

    if (flipX && flipY) {
        angle = fmodf(angle + 180.0f, 360.0f);
    } else if (flipX) {
        angle = 180.0f - angle;
    } else if (flipY) {
        angle = -angle;
    }

    // Normalize again
    angle = positiveFmodf(angle, 360);

    return angle;
}

float adjustAngleY(float angle, int flipX) {
    angle = positiveFmodf(angle, 360);

    if (flipX) {
        angle = angle + 180;
    }

    angle = positiveFmodf(angle, 360);

    return angle;
}


float squareDistance(float xa, float ya, float xb, float yb) {
	return ((xb - xa) * (xb - xa)) + ((yb - ya) * (yb - ya));
}

float randomf() {
    return (float)rand() / __RAND_MAX;
}

float randomFloat(float min, float max) {
    return ((max - min) * ((float)rand() / __RAND_MAX)) + min;
}

int randomInt(int min, int max) {
    return rand() % (max + 1 - min) + min;
}

float reflect(float x, float min, float max) {
    float l = max - min;
    if (l <= 0.0f) return min;

    float t = x - min;

    float period = 2.0f * l;
    float r = fmodf(t, period);
    if (r < 0.0f) r += period;

    if (r > l)
        r = period - r;

    return min + r;
}
bool platformLaunch(float previous, float current, float velocity, float dt, float *launch) {
    if (dt <= 0.f || previous <= 5.f * 54.f || previous <= current ||
        previous - current <= .5f * 60.f * 54.f * dt) return false;
    float speed = roundf(fminf(previous, 20.f * 54.f) * (1000.f / 54.f)) * (54.f / 1000.f);
    if (speed < velocity) return false;
    *launch = speed;
    return true;
}

}
