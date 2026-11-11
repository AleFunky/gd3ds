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

typedef struct Matrix2x2 {
    float m[2][2];
} Matrix2x2;

Matrix2x2 matrix2x2_identity(void);
Matrix2x2 matrix2x2_rotation(float radians);
Matrix2x2 matrix2x2_multiply(Matrix2x2 a, Matrix2x2 b);
Vec2D matrix2x2_transform(Matrix2x2 matrix, Vec2D vector);
Vec2D affine_transform_point(Matrix2x2 matrix, Vec2D offset, Vec2D point);
void compose_affine_transform(Matrix2x2 *current_matrix, Vec2D *current_offset, Matrix2x2 next_matrix, Vec2D next_offset);

// Vector math
Vec2D add_vec(Vec2D a, Vec2D b);
Vec2D sub_vec(Vec2D a, Vec2D b);
Vec2D multiply_vec(Vec2D v, float val);
Vec2D normalize(Vec2D v);
float len_vec(Vec2D v);
float dot_vec(Vec2D a, Vec2D b);
Vec2D perpendicular(Vec2D v);
float square_dist_vec(const Vec2D* a, const Vec2D* b);


float clampf(float d, float min, float max);
float positive_fmodf(float n, float divisor);
Color color_lerp(Color color1, Color color2, float fraction);
u32 color_lerp_u32(u32 color1, u32 color2, float fraction);
float map_range(float val, float min1, float max1, float min2, float max2);
float ease_out(float current, float target, float smoothing);
float adjust_angle(float angle, int flipX, int flipY);
float ip1_ceilf(float x);
float iSlerp(float a, float b, float ratio, float dt);
float slerp_fancy(float fromAngle, float toAngle, float t);
float adjust_angle_y(float angle, int flipX);
float square_distance(float xa, float ya, float xb, float yb);

float reflect(float x, float min, float max);

float randomf();
float random_float(float min, float max);
int random_int(int min, int max);
bool platform_launch(float previous, float current, float velocity, float dt, float *launch);
