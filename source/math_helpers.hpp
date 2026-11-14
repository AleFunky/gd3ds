#pragma once
#include <citro2d.h>
#include "triggers.hpp"
#include "player/collision.hpp"

#define RGB8(r, g, b) (u32)(r | (g << (u32)8) | (b << (u32)16))
#define RGBA8(r, g, b, a) (u32)(a | (b << (u32)8) | (g << (u32)16) | (r << (u32)24))
#define ABGR8(r, g, b, a) (u32)(r | (g << (u32)8) | (b << (u32)16) | (a << (u32)24))

#define R_ABGR8(color) (color & 0xff)
#define G_ABGR8(color) ((color >> 8) & 0xff)
#define B_ABGR8(color) ((color >> 16) & 0xff)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(x, lower, upper) (MIN((upper), MAX((x), (lower))))
#define WRAP(x, min, max)	\
	( (x)>=(max) ? (x)+(min)-(max) : ( ((x)<(min)) ? (x)+(max)-(min) : (x) ) )

#define DegToRad(a)   ( (a) *  0.01745329252f )
#define RadToDeg(a) ( (a) * 57.29577951f )

#define STEPS_HZ 240

#define CCRANDOM_0_1() ((float)rand()/RAND_MAX)

namespace gd3ds
{

struct Matrix2x2 {
    float m[2][2];
};

Matrix2x2 matrix2x2Identity(void);
Matrix2x2 matrix2x2Rotation(float radians);
Matrix2x2 matrix2x2Multiply(Matrix2x2 a, Matrix2x2 b);
Vec2D matrix2x2Transform(Matrix2x2 matrix, Vec2D vector);
Vec2D affineTransformPoint(Matrix2x2 matrix, Vec2D offset, Vec2D point);
void composeAffineTransform(Matrix2x2 *currentMatrix, Vec2D *currentOffset, Matrix2x2 nextMatrix, Vec2D nextOffset);

// Vector math
Vec2D addVec(Vec2D a, Vec2D b);
Vec2D subVec(Vec2D a, Vec2D b);
Vec2D multiplyVec(Vec2D v, float val);
Vec2D normalize(Vec2D v);
float lenVec(Vec2D v);
float dotVec(Vec2D a, Vec2D b);
Vec2D perpendicular(Vec2D v);
float squareDistVec(const Vec2D* a, const Vec2D* b);


float clampf(float d, float min, float max);
float positiveFmodf(float n, float divisor);
Color colorLerp(Color color1, Color color2, float fraction);
u32 colorLerpU32(u32 color1, u32 color2, float fraction);
float mapRange(float val, float min1, float max1, float min2, float max2);
float easeOut(float current, float target, float smoothing);
float adjustAngle(float angle, int flipX, int flipY);
float ip1Ceilf(float x);
float iSlerp(float a, float b, float ratio, float dt);
float slerpFancy(float fromAngle, float toAngle, float t);
float adjustAngleY(float angle, int flipX);
float squareDistance(float xa, float ya, float xb, float yb);

float reflect(float x, float min, float max);

float randomf();
float randomFloat(float min, float max);
int randomInt(int min, int max);
bool platformLaunch(float previous, float current, float velocity, float dt, float *launch);

}
