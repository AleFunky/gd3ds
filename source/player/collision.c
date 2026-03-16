#include "collision.h"
#include "player.h"
#include <math.h>
#include "state.h"
#include <citro3d.h>
#include "graphics.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "slope.h"

const float jump_heights_table[SPEED_COUNT][JUMP_TYPES_COUNT][GAMEMODE_COUNT][2] = {
    { // SLOW               CUBE                   SHIP                  BALL                    UFO                 WAVE   },
    /* YELLOW PAD */ {{875.64,   691.2},    {432,      508.248},  {518.4,       414.72002},   {573.48,   458.784},  {0, 0} },
    /* YELLOW ORB */ {{585.12,   458.784},  {573.48,   458.784},  {401.435993,  321.148795},  {573.48,   458.784},  {0, 0} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-229.392, -183.519}, {-160.574397, -128.463298}, {-229.392, -183.519}, {0, 0} },
    /* BLUE ORB   */ {{-229.392, -183.519}, {-229.392, -183.519}, {-160.574397, -128.463298}, {-229.392, -183.519}, {0, 0} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0} },
    /* PINK ORB   */ {{412.884,  330.318},  {212.166,  169.776},  {309.090595,  247.287596},  {240.84,   192.672},  {0, 0} },
    },
    { // NORMAL
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0} },
    /* YELLOW ORB */ {{603.72,   482.976},  {603.72,   482.976},  {422.60399,   338.08319},   {603.72,   482.976},  {0, 0} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0} },
    /* BLUE ORB   */ {{-241.488, -193.185}, {-241.488, -193.18},  {-169.04160,  -135.2295},   {-241.488, -193.185}, {0, 0} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0} },
    /* PINK ORB   */ {{434.7,    347.76},   {223.398,  178.686},  {325.42019,   260.3286},    {258.984,  207.198},  {0, 0} },
    },
    { // FAST
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0} },
    /* YELLOW ORB */ {{616.68,   481.734},  {616.68,   481.734},  {431.67599,   345.34079},   {616.68,   481.734},  {0, 0} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0} },
    /* BLUE ORB   */ {{-246.672, -197.343}, {-246.672, -197.343}, {-172.6704,   -138.1401},   {-246.672, -197.343}, {0, 0} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0} },
    /* PINK ORB   */ {{443.988,  355.212},  {228.15,   182.52},   {332.37539,   265.923},     {258.984,  207.198},  {0, 0} },
    },
    { // FASTER
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0} },
    /* YELLOW ORB */ {{606.42,   485.136},  {606.42,   485.136},  {424.493993,  339.59519},   {606.42,   485.136},  {0, 0} },
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0} },
    /* BLUE ORB   */ {{-242.568, -194.049}, {-242.568, -194.049}, {-169.7976,   -135.8343},   {-242.568, -194.049}, {0, 0} },
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0} },
    /* PINK ORB   */ {{436.644,  349.272},  {224.37,   179.496},  {326.85659,   261.5004},    {254.718,  203.742},  {0, 0} },
    }
};

const int dual_gamemode_heights[GAMEMODE_COUNT] = {
    9,  // Cube
    10, // Ship
    9,  // Ball
    10, // Ufo
    10 // Wave
};

void flip_other_player(int current_player) {
    if (state.dual && state.player.gamemode == state.player2.gamemode && state.player.upside_down == state.player2.upside_down) {
        if (current_player == 0) {
            state.player2.upside_down = !state.player.upside_down;
            state.player2.vel_y /= -2;
            state.player2.ceiling_inv_time = 0.1f;
        } else {
            state.player.upside_down = !state.player2.upside_down;
            state.player.vel_y /= -2;
            state.player.ceiling_inv_time = 0.1f;
        }
    }
}

void set_dual_bounds() {
    int height = MAX(dual_gamemode_heights[state.player.gamemode],
                 dual_gamemode_heights[state.player2.gamemode]);

    float in_block_y = fmodf(state.dual_portal_y, 30);

    int ground_offset = (ceilf(((float) height + 1) / 2) - 1) * 30;
    state.ground_y = fmaxf(0, floorf((state.dual_portal_y - ground_offset) / 30.f)) * 30;


    // Shift down if odd height and in the top half
    if (height % 2 != 0) {
        if (in_block_y < 15) {
            state.ground_y = fmaxf(0, state.ground_y - 30);
        }
    }

    state.ceiling_y = state.ground_y + (height * 30.f);
    set_intended_ceiling();
}

void setup_dual() {
    memcpy(&state.player2, &state.player, sizeof(Player));
    state.player2.upside_down = state.player.upside_down ^ 1;
    set_dual_bounds();
}


void handle_special_hitbox(Player *player, int obj, const ObjectHitbox *hitbox) {
    switch (objects.id[obj]) {
        case YELLOW_PAD:
            if (!GET_ACTIVATED(obj)) {
                player->vel_y = jump_heights_table[state.speed][JUMP_YELLOW_PAD][player->gamemode][player->mini];
                player->on_ground = false;
                player->inverse_rotation = false;
                player->left_ground = true;
                SET_ACTIVATED(obj, true);
            }
            break;
        case PINK_PAD:
            if (!GET_ACTIVATED(obj)) {
                player->vel_y = jump_heights_table[state.speed][JUMP_PINK_PAD][player->gamemode][player->mini];
                player->on_ground = false;
                player->inverse_rotation = false;
                player->left_ground = true;
                SET_ACTIVATED(obj, true);
            }
            break;
        case BLUE_PAD:
            if (GET_ACTIVATED(obj)) player->gravObj_id = obj;
            else {
                float rotation = adjust_angle(objects.rotation[obj], objects.flippedV[obj], objects.flippedH[obj]);
                if ((rotation < 90 || rotation > 270) && player->upside_down)
                    break;
                    
                if ((rotation > 90 && rotation < 270) && !player->upside_down)
                    break;


                player->left_ground = true;

                player->gravObj_id = obj;

                player->vel_y = jump_heights_table[state.speed][JUMP_BLUE_PAD][player->gamemode][player->mini];
                player->upside_down ^= 1;
                flip_other_player(state.current_player);
                player->on_ground = false;
                player->inverse_rotation = false;

                SET_ACTIVATED(obj, true);
            }
            break;
        case YELLOW_ORB:
            if (!GET_ACTIVATED(obj) && (state.input.holdJump) && player->buffering_state == BUFFER_READY) {
                player->vel_y = jump_heights_table[state.speed][JUMP_YELLOW_ORB][player->gamemode][player->mini];
                
                player->ball_rotation_speed = -1.f;
                
                player->on_ground = false;
                player->on_ceiling = false;
                player->inverse_rotation = false;
                player->left_ground = true;
                player->buffering_state = BUFFER_END;
                
                SET_ACTIVATED(obj, true);
            } 
            break;
        case PINK_ORB:
            if (!GET_ACTIVATED(obj) && (state.input.holdJump) && player->buffering_state == BUFFER_READY) {
                player->vel_y = jump_heights_table[state.speed][JUMP_PINK_ORB][player->gamemode][player->mini];
                
                player->ball_rotation_speed = -1.f;
                
                player->on_ground = false;
                player->on_ceiling = false;
                player->inverse_rotation = false;
                player->left_ground = true;
                player->buffering_state = BUFFER_END;
                
                SET_ACTIVATED(obj, true);
            } 
            break;
        case BLUE_ORB:
            if (!GET_ACTIVATED(obj) && (state.input.holdJump) && player->buffering_state == BUFFER_READY) {    
                player->gravObj_id = obj;
                
                player->vel_y = jump_heights_table[state.speed][JUMP_BLUE_ORB][player->gamemode][player->mini];
                player->upside_down ^= 1;

                flip_other_player(state.current_player);
                
                player->ball_rotation_speed = -1.f;
                
                player->on_ground = false;
                player->on_ceiling = false;
                player->inverse_rotation = false;
                player->left_ground = true;
                player->buffering_state = BUFFER_END;
                player->ufo_last_y = player->y;

                SET_ACTIVATED(obj, true);
            } 
            break;
        case BLUE_GRAVITY_PORTAL:
            player->gravObj_id = obj;
            if (!GET_ACTIVATED(obj)) {
                player->ceiling_inv_time = 0.1f;
                if (player->upside_down) {
                    player->vel_y /= -2;
                    player->upside_down = false;
                    player->inverse_rotation = false;
                    player->snap_rotation = true;
                    flip_other_player(state.current_player);
                    player->left_ground = true;
                }
                SET_ACTIVATED(obj, true);
            } 
            break;
        case YELLOW_GRAVITY_PORTAL:
            player->gravObj_id = obj;
            if (!GET_ACTIVATED(obj)) {
                player->ceiling_inv_time = 0.1f;
                if (!player->upside_down) {
                    player->vel_y /= -2;
                    player->upside_down = true;
                    player->inverse_rotation = false;
                    player->snap_rotation = true;
                    flip_other_player(state.current_player);
                    player->left_ground = true;
                }
                SET_ACTIVATED(obj, true);
            } 
            break;

        case BIG_PORTAL:
            if (!GET_ACTIVATED(obj)) {
                set_mini(player, false);

                SET_ACTIVATED(obj, true);
            }
            break;        

        case MINI_PORTAL:
            if (!GET_ACTIVATED(obj)) {
                set_mini(player, true);

                SET_ACTIVATED(obj, true);
            }
            break;

        case SLOW_SPEED_PORTAL:
            if (!GET_ACTIVATED(obj)) {
                state.speed = SPEED_SLOW;
                SET_ACTIVATED(obj, true);
            }
            break;
        case NORMAL_SPEED_PORTAL:
            if (!GET_ACTIVATED(obj)) {
                state.speed = SPEED_NORMAL;
                SET_ACTIVATED(obj, true);
            }
            break;
        case FAST_SPEED_PORTAL:
            if (!GET_ACTIVATED(obj)) {
                state.speed = SPEED_FAST;
                SET_ACTIVATED(obj, true);
            }
            break;
        case FASTER_SPEED_PORTAL:
            if (!GET_ACTIVATED(obj)) {
                state.speed = SPEED_FASTER;
                SET_ACTIVATED(obj, true);
            }
            break;
        case CUBE_PORTAL: 
            if (!GET_ACTIVATED(obj)) {
                state.ground_y = 0;
                state.ceiling_y = 999999;
                if (player->gamemode != GAMEMODE_PLAYER) {
                    if (player->gamemode != GAMEMODE_PLAYER_BALL) {
                        player->vel_y /= 2;
                    }

                    if (player->gamemode == GAMEMODE_DART) player->vel_y *= 0.9f;
                    
                    player->ceiling_inv_time = 0.1f;
                    player->snap_rotation = true;
                    set_gamemode(player, GAMEMODE_PLAYER);
                    flip_other_player(state.current_player ^ 1);

                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                SET_ACTIVATED(obj, true);
            }
            break;
        case SHIP_PORTAL: 
            if (!GET_ACTIVATED(obj)) {
                state.ground_y = fmaxf(0, ip1_ceilf((objects.y[obj] - 180) / 30.f)) * 30;
                state.ceiling_y = state.ground_y + 300;
                set_intended_ceiling();

                if (player->gamemode != GAMEMODE_SHIP) {
                    if (player->gamemode == GAMEMODE_DART) player->vel_y *= 0.9f;
                    player->vel_y /= (player->gamemode == GAMEMODE_BIRD || player->gamemode == GAMEMODE_DART) ? 4 : 2;
                    
                    set_gamemode(player, GAMEMODE_SHIP);
                    player->inverse_rotation = false;
                    player->snap_rotation = true;
                    flip_other_player(state.current_player ^ 1);
                    
                    float min = player->mini ? -406.566f : -345.6f;
                    float max = player->mini ? 508.248f : 432.0f;

                    if (player->vel_y < min) {
                        player->vel_y = min;
                    } else if (player->vel_y > max) {
                        player->vel_y = max;
                    }
                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                SET_ACTIVATED(obj, true);
            }
            break;
        case BALL_PORTAL: 
            if (!GET_ACTIVATED(obj)) {
                state.ground_y = fmaxf(0, ip1_ceilf((objects.y[obj] - 150) / 30.f)) * 30;
                state.ceiling_y = state.ground_y + 240;
                set_intended_ceiling();

                if (player->gamemode != GAMEMODE_PLAYER_BALL) {
                    player->ball_rotation_speed = -1.f;

                    switch (player->gamemode) {
                        case GAMEMODE_DART:
                            player->vel_y *= 0.9f;
                            player->vel_y /= 2;
                        case GAMEMODE_SHIP:
                        case GAMEMODE_BIRD:
                            player->vel_y /= 2;
                            break;
                    }
                    
                    set_gamemode(player, GAMEMODE_PLAYER_BALL);
                    player->inverse_rotation = false;
                    player->snap_rotation = true;
                    flip_other_player(state.current_player ^ 1);
                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                SET_ACTIVATED(obj, true);
            }
            break;
        case UFO_PORTAL:
            if (!GET_ACTIVATED(obj)) {
                state.ground_y = fmaxf(0, ip1_ceilf((objects.y[obj] - 180) / 30.f)) * 30;
                state.ceiling_y = state.ground_y + 300;
                set_intended_ceiling();
                
                if (player->gamemode != GAMEMODE_BIRD) {
                    if (player->gamemode == GAMEMODE_DART) player->vel_y *= 0.9f;
                    player->vel_y /= (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_DART) ? 4 : 2;
                    set_gamemode(player, GAMEMODE_BIRD);
                    player->ufo_last_y = player->y;
                    player->inverse_rotation = false;
                    player->snap_rotation = true;
                    flip_other_player(state.current_player ^ 1);

                    if (state.old_player.gamemode == GAMEMODE_PLAYER || state.old_player.gamemode == GAMEMODE_SHIP || state.old_player.gamemode == GAMEMODE_DART) {
                        player->buffering_state = BUFFER_READY;
                    }
                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                SET_ACTIVATED(obj, true);
            }
            break;
        case WAVE_PORTAL:
            if (!GET_ACTIVATED(obj)) {
                state.ground_y = fmaxf(0, ip1_ceilf((objects.y[obj] - 180) / 30.f)) * 30;
                state.ceiling_y = state.ground_y + 300;
                set_intended_ceiling();

                if (player->gamemode != GAMEMODE_DART) {
                    set_gamemode(player, GAMEMODE_DART);
                    player->inverse_rotation = false;
                    player->snap_rotation = true;
                    flip_other_player(state.current_player ^ 1);
                }

                if (state.dual) {
                    set_dual_bounds();
                } 
                SET_ACTIVATED(obj, true);
            }
            break;
        case DUAL_PORTAL:
            player->gravObj_id = obj;
            if (!GET_ACTIVATED(obj) && !state.dual) {
                player->ceiling_inv_time = 0.1f;
                state.dual = true;
                state.dual_portal_y = objects.y[obj];
                setup_dual();

                SET_ACTIVATED(obj, true);
            }
            break;

        case DIVORCE_PORTAL:
            if (!GET_ACTIVATED(obj)) {

                state.dual = false;
                SET_ACTIVATED(obj, true);

                if (state.current_player == 1) {
                    memcpy(&state.player, player, sizeof(Player));
                }
                
                switch (state.player.gamemode) {
                    case GAMEMODE_PLAYER:
                        state.ground_y = 0;
                        state.ceiling_y = 999999;
                        break;
                    case GAMEMODE_SHIP:
                    case GAMEMODE_BIRD:
                        state.ceiling_y = state.ground_y + 300;
                        set_intended_ceiling();
                        break;
                    case GAMEMODE_PLAYER_BALL:
                        state.ceiling_y = state.ground_y + 240;
                        set_intended_ceiling();
                }
            }
            break;
    }
    if (!GET_COLLIDED(obj)) SET_HITBOX_COUNTER(obj, GET_HITBOX_COUNTER(obj) + 1); 
}

void get_corners(float cx, float cy, float w, float h, float angle, Vec2D out[4]) {
    float hw = w / 2.0f, hh = h / 2.0f;
    angle = -angle; // Make it clockwise
    float rad = C3D_AngleFromDegrees(angle);
    float cos_a = cosf(rad), sin_a = sinf(rad);
    

    float local[4][2] = {
        { -hw, -hh },
        {  hw, -hh },
        {  hw,  hh },
        { -hw,  hh }
    };
    for (int i = 0; i < 4; ++i) {
        out[i].x = cx + local[i][0] * cos_a - local[i][1] * sin_a;
        out[i].y = cy + local[i][0] * sin_a + local[i][1] * cos_a;
    }
}

static bool sat_overlap(const Vec2D a[4], const Vec2D b[4]) {
    // Test all axes (normals of edges)
    for (int shape = 0; shape < 2; ++shape) {
        const Vec2D *verts = (shape == 0) ? a : b;
        for (int i = 0; i < 4; ++i) {
            // Edge from verts[i] to verts[(i+1)%4]
            float dx = verts[(i+1)%4].x - verts[i].x;
            float dy = verts[(i+1)%4].y - verts[i].y;
            // Normal axis
            float ax = -dy, ay = dx;

            // Project both shapes onto axis
            float minA = INFINITY, maxA = -INFINITY;
            float minB = INFINITY, maxB = -INFINITY;
            for (int j = 0; j < 4; ++j) {
                float projA = a[j].x * ax + a[j].y * ay;
                float projB = b[j].x * ax + b[j].y * ay;
                if (projA < minA) minA = projA;
                if (projA > maxA) maxA = projA;
                if (projB < minB) minB = projB;
                if (projB > maxB) maxB = projB;
            }
            // If projections do not overlap, there is a separating axis
            if (maxA <= minB || maxB <= minA) return false;
        }
    }
    return true;
}

bool intersect(float x1, float y1, float w1, float h1, float angle1,
               float x2, float y2, float w2, float h2, float angle2) {
    float big = fmaxf(w1, h1) + fmaxf(w2, h2);
    if (fabsf(x1 - x2) > big || fabsf(y1 - y2) > big) {
        return false;
    }
    
    Vec2D rect1[4], rect2[4];
    get_corners(x1, y1, w1, h1, angle1, rect1);
    get_corners(x2, y2, w2, h2, angle2, rect2);
    return sat_overlap(rect1, rect2);
}

bool intersect_rect_circle(float rx, float ry, float rw, float rh, float rangle,
                          float cx, float cy, float cradius) {
    // If centers are too far apart, no collision
    float max_dim = fmaxf(rw, rh);
    float max_dist = (max_dim / 2.0f) + cradius;
    if (fabsf(rx - cx) > max_dist || fabsf(ry - cy) > max_dist) {
        return false;
    }

    // Transform circle center into rectangle's local space
    float rad = -C3D_AngleFromDegrees(rangle); // negative for inverse rotation
    float cos_a = cosf(rad), sin_a = sinf(rad);

    float local_cx = cos_a * (cx - rx) - sin_a * (cy - ry) + rx;
    float local_cy = sin_a * (cx - rx) + cos_a * (cy - ry) + ry;

    // Rectangle bounds
    float left   = rx - rw / 2.0f;
    float right  = rx + rw / 2.0f;
    float top    = ry - rh / 2.0f;
    float bottom = ry + rh / 2.0f;

    // Find closest point on rectangle to circle center
    float closest_x = fmaxf(left, fminf(local_cx, right));
    float closest_y = fmaxf(top,  fminf(local_cy, bottom));

    // Distance from circle center to closest point
    float dx = local_cx - closest_x;
    float dy = local_cy - closest_y;
    float dist_sq = dx * dx + dy * dy;

    return dist_sq <= cradius * cradius;
}
// Dot product helper
float dot(float x1, float y1, float x2, float y2) {
    return x1 * x2 + y1 * y2;
}

bool circle_rect_collision(float cx, float cy, float radius,
                           float x1, float y1, float x2, float y2) {
    // Vector from point 1 to circle center
    float dx = x2 - x1;
    float dy = y2 - y1;
    float fx = cx - x1;
    float fy = cy - y1;

    float len_sq = dx * dx + dy * dy;
    float t = dot(fx, fy, dx, dy) / len_sq;

    // Clamp t to the [0,1] range to stay within the segment
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;

    // Closest point on segment
    float closestX = x1 + t * dx;
    float closestY = y1 + t * dy;

    // Distance from circle center to closest point
    float distX = cx - closestX;
    float distY = cy - closestY;
    float distSq = distX * distX + distY * distY;

    return distSq <= radius * radius;
}

void handle_collision(Player *player, int obj, const ObjectHitbox *hitbox) {
    InternalHitbox internal = player->internal_hitbox;

    int clip = (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_BIRD) ? 7 : 10;
    switch (hitbox->collision_type) {
        //case HITBOX_BREAKABLE_BLOCK:
        case HITBOX_SOLID: 
            bool gravSnap = false;

            clip += fabsf(player->vel_y) * STEPS_DT;
            
            if (player->slope_data.slope_id >= 0) {
                return;
            }
            
            // Collide with slope if object is an slope
            if (hitbox->type == COLLISION_SLOPE) {
                slope_collide(obj, player);
                break;
            }
            
            if (player->gravObj_id >= 0 && GET_HITBOX_COUNTER(player->gravObj_id) == 1) {
                // Only do the funny grav snap if player is touching a gravity object and internal hitbox is touching block
                bool internalCollidingBlock = intersect(
                    player->x, player->y, internal.width, internal.height, 0, 
                    objects.x[obj], objects.y[obj], hitbox->width, hitbox->height, objects.rotation[obj]
                );

                gravSnap = (!state.old_player.on_ground || player->ceiling_inv_time > 0) && internalCollidingBlock && obj_gravBottom(player, obj) - gravInternalBottom(player) <= clip;
            }
            
            
            bool slope_height_check = false;
            if (player->touching_slope) {
                if (grav_slope_orient(player->potentialSlope_id, player) == ORIENT_NORMAL_DOWN) {
                    slope_height_check = gravBottom(player) < grav(player, objects.y[player->potentialSlope_id]);
                } else if (grav_slope_orient(player->potentialSlope_id, player) == ORIENT_UD_DOWN) {
                    slope_height_check = gravTop(player) > grav(player, objects.y[player->potentialSlope_id]);
                }
            }
            bool slope_condition = player->touching_slope && !slope_touching(player->potentialSlope_id, player) && slope_height_check && (objects.orientation[player->potentialSlope_id] == ORIENT_NORMAL_DOWN || objects.orientation[player->potentialSlope_id] == ORIENT_UD_DOWN);

            // Snap the player to the potential slope when the player is touching the slope
            if (player->touching_slope && slope_touching(player->potentialSlope_id, player) && slope_height_check) {
                slope_collide(player->potentialSlope_id, player);
                break;
            }
            
            bool safeZone = player->mini && ((obj_gravTop(player, obj) - gravBottom(player) <= clip) || (gravTop(player) - obj_gravBottom(player, obj) <= clip));
            
            if ((player->gamemode == GAMEMODE_DART || (!gravSnap && !safeZone)) && intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                objects.x[obj], objects.y[obj], hitbox->width, hitbox->height, objects.rotation[obj]
            )) {
                if (objects.id[obj] == BREAKABLE_BLOCK) {
                    // Spawn breakable brick particles
                    objects.id[obj] = 0;
                } else {
                    // Not a brick, die
                    state.dead = true;
                }
            // Check snap for player bottom
            } else if (obj_gravTop(player, obj) - gravBottom(player) <= clip && player->vel_y <= 0 && !slope_condition && player->gamemode != GAMEMODE_DART) {
                player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
                if (player->vel_y <= 0) player->vel_y = 0;
                player->on_ground = true;
                player->inverse_rotation = false;
                player->time_since_ground = 0;
            // Check snap for player top
            } else if (player->gamemode != GAMEMODE_DART) {
                // Ufo can break breakable blocks from above, so dont use as a ceiling
                if (player->gamemode == GAMEMODE_BIRD && objects.id[obj] == BREAKABLE_BLOCK) {
                    break;
                }
                // Behave normally
                if (player->gamemode != GAMEMODE_PLAYER || gravSnap) {
                    if (((gravTop(player) - obj_gravBottom(player, obj) <= clip && player->vel_y >= 0) || gravSnap) && !slope_condition) {
                        if (!gravSnap) player->on_ceiling = true;
                        else player->vel_y = 0;
                        player->inverse_rotation = false;
                        player->time_since_ground = 0;
                        player->ceiling_inv_time = 0;
                        player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
                        if (player->vel_y >= 0) player->vel_y = 0;
                    }
                }
            }
            break;
        case HITBOX_HAZARD:
            state.dead = true;
            break;
        case HITBOX_SPECIAL:
            handle_special_hitbox(player, obj, hitbox);
            break;
    }
}

void collide_with_obj(Player *player, int obj) {
    int obj_id = objects.id[obj];
    const ObjectHitbox *hitbox = game_objects[obj_id].hitbox;

    if (!hitbox) return;

    //number_of_collisions_checks++;

    float x = objects.x[obj];
    float y = objects.y[obj];
    float width = objects.width[obj];
    float height = objects.height[obj];

    if (hitbox->type == COLLISION_CIRCLE) {
        if (intersect_rect_circle(
            player->x, player->y, player->width, player->height, player->rotation, 
            x, y, hitbox->width
        )) {
            handle_collision(player, obj, hitbox);
            SET_COLLIDED(obj, true);
            //number_of_collisions++;
        } else {
            SET_COLLIDED(obj, false);
        }
    } else {
        float obj_rot = normalize_angle(objects.rotation[obj]);

        if (hitbox->collision_type == HITBOX_SOLID) {
            obj_rot = 0;
        }

        float rotation = (obj_rot == 0 || obj_rot == 90 || obj_rot == 180 || obj_rot == 270) ? 0 : player->rotation;
        
        bool checkColl = intersect(
            player->x, player->y, player->width, player->height, rotation, 
            x, y, width, height, obj_rot
            
        );

        // Rotated hitboxes must also collide with the unrotated hitbox
        if (rotation != 0) {
            checkColl = checkColl && intersect(
                player->x, player->y, player->width, player->height, 0, 
                x, y, width, height, obj_rot
            );
        }

        if (checkColl) {
            handle_collision(player, obj, hitbox);
            SET_COLLIDED(obj, true);
            //number_of_collisions++;
        } else {
            SET_COLLIDED(obj, false);
        }
    }
}

void collide_with_slope(Player *player, int obj, bool has_slope) {
    const ObjectHitbox *hitbox = game_objects[objects.id[obj]].hitbox;
    
    if (!hitbox) return;
    
    float width = hitbox->width;
    float height = hitbox->height;

    if (intersect(
        player->x, player->y, player->width, player->height, 0, 
        objects.x[obj], objects.y[obj], width, height, objects.rotation[obj]
    )) {
        // The same check in handle_collision
        if (has_slope) {
            float bottom = gravBottom(player) + sinf(slope_angle(player->slope_data.slope_id, player)) * player->height / 2;
            if (obj_gravTop(player, obj) - bottom < 2)
                return;
        }
        slope_collide(obj, player);
    }
}

int slope_buffer[MAX_COLLIDED_OBJECTS];
int slope_count = 0;

int block_buffer[MAX_COLLIDED_OBJECTS];
int block_count = 0;

int hazard_buffer[MAX_COLLIDED_OBJECTS];
int hazard_count = 0;

void collide_with_objects(Player *player) {
    //number_of_collisions = 0;
    //number_of_collisions_checks = 0;

    int sx = (int)(player->x / SECTION_SIZE);
    
    for (int dx = -1; dx <= 1; dx++) {
        Section *sec = get_or_create_section(sx + dx);
        for (int i = 0; i < sec->object_count; i++) {
            int obj = sec->objects[i];
            const ObjectHitbox *hitbox = game_objects[objects.id[obj]].hitbox;

            if (!hitbox) continue;
            
            // Save some types to buffer, so they can be checked in a type order
            if (hitbox->collision_type == HITBOX_SOLID) {
                if (hitbox->type == COLLISION_SLOPE) {
                    slope_buffer[slope_count++] = obj;
                } else {
                    block_buffer[block_count++] = obj;
                }
            } else if (hitbox->collision_type == HITBOX_HAZARD) {
                hazard_buffer[hazard_count++] = obj;
            } else { // HITBOX_SPECIAL
                collide_with_obj(player, obj);
            }
        }
    }

    if (player->left_ground) {
        clear_slope_data(player);
    }

    float closestDist = 999999.f;
    // Detect if touching slope
    for (int i = 0; i < slope_count; i++) {
        int obj = slope_buffer[i];
        if (intersect(
            player->x, player->y, player->width, player->height, 0, 
            objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], objects.rotation[obj]
        )) {
            float dist = fabsf(objects.y[obj] - player->y);
            if (dist < closestDist) {
                player->touching_slope = true;
                player->potentialSlope_id = obj;
                closestDist = dist; 
            }
        }
    }

    for (int i = 0; i < block_count; i++) {
        int obj = block_buffer[i];
        collide_with_obj(player, obj);
    }

    bool has_slope = player->slope_data.slope_id >= 0;
    for (int i = 0; i < slope_count; i++) {
        int obj = slope_buffer[i];
        collide_with_slope(player, obj, has_slope);
    }
    
    for (int i = 0; i < hazard_count; i++) {
        int obj = hazard_buffer[i];
        collide_with_obj(player, obj);
    }

    player->touching_slope = false;
    slope_count = 0;
    block_count = 0;
    hazard_count = 0;
}