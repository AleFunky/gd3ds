#include "slope.h"
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

const float falls[SPEED_COUNT] = {
    226.044054,
    280.422108,
    348.678108,
    421.200108
};


void clear_slope_data(Player *player) {
    player->slope_data.slope_id = -1;
    player->slope_data.snapDown = false;
}

int grav_slope_orient(int obj, Player *player) {
    int orient = objects.orientation[obj];

    if (player->upside_down) {
        // Flip vertically slope orientation
        if (orient == ORIENT_UD_UP)
			orient = ORIENT_NORMAL_UP;
		else if (orient == ORIENT_UD_DOWN)
			orient = ORIENT_NORMAL_DOWN;
		else if (orient == ORIENT_NORMAL_UP)
			orient = ORIENT_UD_UP;
		else if (orient == ORIENT_NORMAL_DOWN)
			orient = ORIENT_UD_DOWN;
    }
    return orient;
}

bool is_spike_slope(int obj) {
    switch (objects.id[obj]) {
        case 363:
        case 364:
        case 366:
        case 367:
            return true;
    }
    return false;
}

float slope_angle(int obj, Player *player) {
    float angle = atanf((float) objects.height[obj] / objects.width[obj]);
    int orient = grav_slope_orient(obj, player);
    if (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) {
        angle = -angle;
    }

    return angle;
}

float get_slope_angle(int obj) {
    float angle = atanf((float) objects.height[obj] / objects.width[obj]);
    return angle;
}

float slope_snap_angle(int obj, Player *player) {
    float angle = slope_angle(obj, player);
    int orient = objects.orientation[obj];

    if (orient == ORIENT_NORMAL_UP) angle = -fabsf(angle);
    if (orient == ORIENT_NORMAL_DOWN) angle = fabsf(angle);

    return angle;
}

float expected_slope_y(int obj, Player *player) {
    int flipping = grav_slope_orient(obj, player) >= ORIENT_UD_DOWN;
    int mult = (player->upside_down ^ flipping) ? -1 : 1;
    
    float angle = slope_angle(obj, player);
    float ydist = mult * player->height * sqrtf(powf(tanf(angle), 2) + 1) / 2;
    float pos_relative = ((float) objects.height[obj] / objects.width[obj]) * (player->x - obj_getLeft(obj));

    float y;
    // Get correct slope y depending on combination of player gravity and slope orientation
    if ((angle > 0) ^ player->upside_down ^ flipping) {
        y = obj_getBottom(obj) + MIN(pos_relative + ydist, objects.height[obj] + player->height / 2);
    } else {
        y = obj_getTop(obj) - MAX(pos_relative - ydist, -player->height / 2);
    }

    // Spike slope has bigger hitbox
    if (is_spike_slope(obj)) {
        y += (objects.orientation[obj] >= ORIENT_UD_DOWN ? -4 : 4);
    }

    return y;

}

void slope_snap_y(int obj, Player *player) {
    int orientation = grav_slope_orient(obj, player);

    switch (orientation) {
        case ORIENT_NORMAL_UP: // Normal - up
            if (player->upside_down) {
                player->y = MAX(obj_getBottom(obj) - player->height / 2, expected_slope_y(obj, player));
            } else {
                player->y = MIN(obj_getTop(obj) + player->height / 2, expected_slope_y(obj, player));
            }

            player->time_since_ground = 0;
            player->on_ground = true;
            snap_player_to_slope(obj, player);
            
            if (player->vel_y < 0) {
                player->vel_y = 0;
            }
            break;
        case ORIENT_NORMAL_DOWN: // Normal - down
            if (player->upside_down) {
                player->y = MIN(expected_slope_y(obj, player), obj_getTop(obj) + player->height / 2);
            } else {
                player->y = MAX(expected_slope_y(obj, player), obj_getBottom(obj) - player->height / 2);
            }
            
            player->time_since_ground = 0;
            player->on_ground = true;
            snap_player_to_slope(obj, player);
            if (player->vel_y < 0) {
                player->vel_y = 0;
            }
            break;
        case ORIENT_UD_DOWN: // Upside down - down
            if (player->upside_down) {
                player->y = MAX(expected_slope_y(obj, player), obj_getBottom(obj) - player->height / 2);
            } else {
                player->y = MIN(expected_slope_y(obj, player), obj_getTop(obj) + player->height / 2);
            }
            
            player->time_since_ground = 0;
            player->on_ceiling = true;
            snap_player_to_slope(obj, player);
            if (player->vel_y > 0) {
                player->vel_y = 0;
            }
            break;
        case ORIENT_UD_UP: // Upside down - up
            if (player->upside_down) {
                player->y = MIN(obj_getTop(obj) + player->height / 2, expected_slope_y(obj, player));
            } else {
                player->y = MAX(obj_getBottom(obj) - player->height / 2, expected_slope_y(obj, player));
            }

            player->time_since_ground = 0;
            player->on_ceiling = true;
            snap_player_to_slope(obj, player);
            
            if (player->vel_y > 0) {
                player->vel_y = 0;
            }
            break;
    }
}

void slope_calc(int obj, Player *player) {
    int orientation = grav_slope_orient(obj, player);
    if (orientation == ORIENT_NORMAL_UP) { // Normal - up
        // Handle leaving slope
        if (!slope_touching(obj, player)) {
            clear_slope_data(player);
            return;
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj)) {
            slope_snap_y(obj, player);
        }

        // Sliding off slope
        if (gravBottom(player) >= obj_gravTop(player, obj)) {
            float vel = 0.9f * MIN(1.12 / slope_angle(obj, player), 1.54) * (objects.height[obj] * player_speeds[state.speed] / objects.width[obj]);
            float time = clampf(10 * (player->timeElapsed - player->slope_data.elapsed), 0.4f, 1.0f);
            
            //float orig = vel;
            if (player->gamemode == GAMEMODE_PLAYER_BALL) {
                vel *= 0.75f;
            }
            
            if (player->gamemode == GAMEMODE_SHIP) {
                vel *= 0.75f;
            }

            if (player->gamemode == GAMEMODE_BIRD) {
                vel *= 0.7499f;
            }

            vel *= time;

            player->vel_y = vel;
            player->inverse_rotation = true;
            player->coyote_slope = player->slope_data;
            player->slope_slide_coyote_time = 2;
            clear_slope_data(player);
        }
    } else if (orientation == ORIENT_NORMAL_DOWN) { // Normal - down
        // Handle leaving slope
        if (player->vel_y > 0) {
            clear_slope_data(player);
            return;
        }

        if (gravBottom(player) != obj_gravTop(player, obj) || player->slope_data.snapDown) {
            slope_snap_y(obj, player);
        }

        if (obj_gravTop(player, obj) <= grav(player, player->y) || getLeft(player) - obj_getRight(obj) > 0) {
            float vel = -falls[state.speed] * ((float) objects.height[obj] / objects.width[obj]);
            player->vel_y = vel;
            clear_slope_data(player);
        }
    } else if (orientation == ORIENT_UD_UP) { // Upside down - up
        // Handle leaving slope
        if (!slope_touching(obj, player)) {
            clear_slope_data(player);
            return;
        }
        
        bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj_id >= 0 && GET_HITBOX_COUNTER(player->gravObj_id) == 1);
        
        if (player->gamemode == GAMEMODE_PLAYER && !gravSnap) {
            state.dead = true;
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj)) {
            slope_snap_y(obj, player);
        }

        // Sliding off slope
        if (gravTop(player) <= obj_gravBottom(player, obj)) {
            float vel = 0.9f * MIN(1.12 / slope_angle(obj, player), 1.54) * (objects.height[obj] * player_speeds[state.speed] / objects.width[obj]);
            float time = clampf(10 * (player->timeElapsed - player->slope_data.elapsed), 0.4f, 1.0f);
            
            //float orig = vel;
            if (player->gamemode == GAMEMODE_PLAYER_BALL) {
                vel *= 0.75f;
            }
            
            if (player->gamemode == GAMEMODE_SHIP) {
                vel *= 0.75f;
            }

            if (player->gamemode == GAMEMODE_BIRD) {
                vel *= 0.7499f;
            }

            vel *= time;

            player->vel_y = -vel;
            
            player->inverse_rotation = true;
            player->coyote_slope = player->slope_data;
            player->slope_slide_coyote_time = 2;
            clear_slope_data(player);
        }
    } else if (orientation == ORIENT_UD_DOWN) { // Upside down - down
        // Handle leaving slope
        if (player->vel_y < 0) {
            clear_slope_data(player);
            return;
        }
        
        bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj_id >= 0 && GET_HITBOX_COUNTER(player->gravObj_id) == 1);
        
        if (player->gamemode == GAMEMODE_PLAYER && !gravSnap) {
            state.dead = true;
        }

        // On slope
        if (gravTop(player) != obj_gravBottom(player, obj) || player->slope_data.snapDown) {
            slope_snap_y(obj, player);
        }

        // Sliding off
        if (obj_gravTop(player, obj) <= grav(player, player->y) || getLeft(player) - obj_getRight(obj) > 0) {
            float vel = falls[state.speed] * ((float) objects.height[obj] / objects.width[obj]);
            player->vel_y = vel;
            clear_slope_data(player);
        }
    }
}


bool player_circle_touches_slope(int obj, Player *player) {
    float x1, y1, x2, y2;
    int orientation = objects.orientation[obj];

    float hw = objects.width[obj] / 2.f, hh = objects.height[obj] / 2.f;

    float player_radius = (player->width - 4) / 2;

    // Collide with hipotenuse
    switch (orientation) {
        case ORIENT_NORMAL_UP:
        case ORIENT_UD_DOWN:
            x1 = objects.x[obj] - hw;
            y1 = objects.y[obj] - hh;
            x2 = objects.x[obj] + hw;
            y2 = objects.y[obj] + hh;
            break;
        case ORIENT_NORMAL_DOWN:
        case ORIENT_UD_UP:
            x1 = objects.x[obj] + hw;
            y1 = objects.y[obj] - hh;
            x2 = objects.x[obj] - hw;
            y2 = objects.y[obj] + hh;
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
            break;
    }
    bool collided_hipo = circle_rect_collision(player->x, player->y, player_radius, x1, y1, x2, y2);

    // Collide with vertical
    switch (orientation) {
        case ORIENT_NORMAL_UP:
        case ORIENT_UD_UP:
            x1 = objects.x[obj] + hw;
            y1 = objects.y[obj] - hh;
            x2 = objects.x[obj] + hw;
            y2 = objects.y[obj]  + hh;
            break;
        case ORIENT_NORMAL_DOWN:
        case ORIENT_UD_DOWN:
            x1 = objects.x[obj] - hw;
            y1 = objects.y[obj]  - hh;
            x2 = objects.x[obj] - hw;
            y2 = objects.y[obj]  + hh;
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
            break;
    }
    
    bool collided_vertical = circle_rect_collision(player->x, player->y, player_radius, x1, y1, x2, y2);

    // Collide with horizontal
    switch (orientation) {
        case ORIENT_NORMAL_UP:
        case ORIENT_NORMAL_DOWN:
            x1 = objects.x[obj] + hw;
            y1 = objects.y[obj]  - hh;
            x2 = objects.x[obj] - hw;
            y2 = objects.y[obj]  - hh;
            break;
        case ORIENT_UD_DOWN:
        case ORIENT_UD_UP:
            x1 = objects.x[obj] + hw;
            y1 = objects.y[obj]  + hh;
            x2 = objects.x[obj] - hw;
            y2 = objects.y[obj]  + hh;
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
            break;
    }
    
    bool collided_horizontal = circle_rect_collision(player->x, player->y, player_radius, x1, y1, x2, y2);

    return collided_vertical | collided_hipo | collided_horizontal;
}

void slope_collide(int obj, Player *player) {
    if (potential_slopes[state.current_player] < MAX_COLLIDED_OBJECTS) {
        potential_slopes_buffer[state.current_player][potential_slopes[state.current_player]++] = obj;
    }
    
    int clip = (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_BIRD) ? 7 : 10;
    int orient = grav_slope_orient(obj, player);  
    int mult = orient >= ORIENT_UD_DOWN ? -1 : 1;

    InternalHitbox internal = player->internal_hitbox;

    bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj_id >= 0 && GET_HITBOX_COUNTER(player->gravObj_id) == 1);

    // Check if player inside slope
    if (orient == ORIENT_NORMAL_UP || orient == ORIENT_UD_UP) {
        bool internalCollidingSlope = intersect(
            player->x, player->y, internal.width, internal.height, 0, 
            obj_getRight(obj), objects.y[obj] , 1, objects.height[obj], 0
        );

        // Die if so
        if (internalCollidingSlope) state.dead = true;
    }

    // Normal slope - resting on bottom
    if (
        state.old_player.slope_data.slope_id < 0 &&
        orient < ORIENT_UD_DOWN && 
        gravTop(player) - obj_gravBottom(player, obj) <= clip + 5 * !player->mini // Remove extra if mini
    ) {
        if (player->gamemode != GAMEMODE_DART && ((player->gamemode != GAMEMODE_PLAYER && (player->vel_y >= 0)) || gravSnap)) {
            player->vel_y = 0;
            if (!gravSnap) player->on_ceiling = true;
            player->time_since_ground = 0;
            player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
        } else {
            bool internalCollidingSlope = intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
            );

            if (internalCollidingSlope) state.dead = true;
        }

        return;
    }

    // Upside down slope - resting on top
    if (
        state.old_player.slope_data.slope_id < 0 &&
        orient >= ORIENT_UD_DOWN && 
        obj_gravTop(player, obj) - gravBottom(player) <= clip + 5 * !player->mini // Remove extra if mini
    ) {
        if (player->gamemode != GAMEMODE_DART && player->vel_y <= 0) {
            player->vel_y = 0;
            if (!gravSnap) player->on_ground = true;
            player->time_since_ground = 0;
            player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
        } else {
            bool internalCollidingSlope = intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
            );

            if (internalCollidingSlope) state.dead = true;
        }
        
        return;
    }

    // Left side collision
    if (
        state.old_player.slope_data.slope_id < 0 && 
        (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) && 
        player->x - obj_getLeft(obj) < 0
    ) {
        // Going from the left
        if (obj_gravTop(player, obj) - gravBottom(player) > clip) {
            bool internalCollidingSlope = intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
            );

            if (internalCollidingSlope) state.dead = true;
            return;
        }
        
        // Touching slope before center is in slope
        if (player->gamemode != GAMEMODE_DART && player->vel_y * mult <= 0) {
            if (orient == ORIENT_NORMAL_DOWN) {
                player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
            } else {
                player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
            }
            player->on_ground = true;
            player->inverse_rotation = false;
            return;
        }
    }

    if (!gravSnap && player->gamemode == GAMEMODE_PLAYER && grav_slope_orient(obj, player) >= 2 && !player_circle_touches_slope(obj, player)) return;

    bool colliding = intersect(
        player->x, player->y, player->width, player->height, 0, 
        objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
    );

    int slope = player->slope_data.slope_id;
    if (
        (slope < 0 || grav_slope_orient(slope, player) == grav_slope_orient(obj, player) ||
            (
                // Check if going from going down to up
                grav_slope_orient(slope, player) != grav_slope_orient(obj, player) && 
                (
                    (grav_slope_orient(slope, player) == ORIENT_NORMAL_DOWN && grav_slope_orient(obj, player) == ORIENT_NORMAL_UP) ||
                    (grav_slope_orient(slope, player) == ORIENT_UD_DOWN && grav_slope_orient(obj, player) == ORIENT_UD_UP)
                )
            ) 
        ) && slope_touching(obj, player) && colliding && obj_gravTop(player, obj) - gravBottom(player) > 1
    ) {
        
        if (slope >= 0 && slope_angle(obj, player) < slope_angle(slope, player)) return;
        float angle = atanf((player->vel_y * STEPS_DT) / (player_speeds[state.speed] * STEPS_DT));
        
        if (orient >= ORIENT_UD_DOWN) angle = -angle;

        bool hasSlope = state.old_player.slope_data.slope_id >= 0;

        // Check if the old slope and this slope have the same orientation, if not, then the player doesn't have an slope
        if (hasSlope && slope >= 0) {
            hasSlope = objects.orientation[state.old_player.slope_data.slope_id] == objects.orientation[slope];
        }
        

        #define SLOPE_EPSILON 3
        bool projectedHit = (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) ? (angle * 1.1f <= slope_angle(obj, player)) : (angle <= slope_angle(obj, player));
        bool clip = slope_touching(obj, player);
        bool snapDown = (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) && player->vel_y * mult > 0 && player->x - obj_getLeft(obj) > 0;

        if ((projectedHit && clip) || snapDown) {
            // If wave, just die, nothing else, wave hates slopes
            if (player->gamemode == GAMEMODE_DART) {
                state.dead = true;
                return;
            }

            if (orient >= ORIENT_UD_DOWN) player->on_ceiling = true;
            else player->on_ground = true;
            
            player->inverse_rotation = false;
            player->slope_data.slope_id = obj;
            slope_snap_y(obj, player);
            snap_player_to_slope(obj, player);

            if (is_spike_slope(obj)) {
                state.dead = true;
            }

            // If player is on an slope that goes down, and is in the top corner, snap down
            if (snapDown && !hasSlope) {
                if (orient == ORIENT_NORMAL_DOWN && player->vel_y <= 0) {
                    player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
                    player->slope_data.snapDown = true;
                    player->vel_y = 0;
                } else if (orient == ORIENT_UD_DOWN && player->vel_y >= 0) {
                    player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
                    player->slope_data.snapDown = true;
                    player->vel_y = 0;
                }
            }

            if (state.old_player.slope_data.slope_id < 0 && player->coyote_slope.slope_id < 0) {
                player->slope_data.elapsed = 0.f;
            }

            // If the player wasn't on an slope, initialize the time elapsed
            if (!player->slope_data.elapsed) {
                Player *other_player = (state.current_player == 0 ? &state.player2 : &state.player);

                // Make both times synced if close enough
                if (other_player->slope_data.slope_id >= 0 && fabsf(player->timeElapsed - other_player->slope_data.elapsed) < 0.10) {
                    player->slope_data.elapsed = other_player->slope_data.elapsed;
                } else {
                    player->slope_data.elapsed = player->timeElapsed;
                }
            }
        } 
    }
}

bool slope_touching(int obj, Player *player) {
    bool hasSlope = player->slope_data.slope_id >= 0;

    // If player is on slope, add a bit of hitbox
    if (hasSlope) {
        int mult = grav_slope_orient(player->slope_data.slope_id, player) >= 2 ? -1 : 1;
        hasSlope = hasSlope && player->vel_y * mult <= 0;
    }
    
    float deg = RadToDeg(fabsf(slope_angle(obj, player)));
    float snap_height = 20 * (deg / 45);
    float min = hasSlope ? -3 : 0;
    
    if (obj_getRight(obj) < getLeft(player)) return false;

    switch (grav_slope_orient(obj, player)) {
        case ORIENT_NORMAL_UP:
        case ORIENT_NORMAL_DOWN:
            float diff = grav(player, expected_slope_y(obj, player)) - grav(player, player->y);
            return diff >= min && diff <= snap_height;
        case ORIENT_UD_UP:
        case ORIENT_UD_DOWN:
            float diff_ud = grav(player, player->y) - grav(player, expected_slope_y(obj, player));  
            return diff_ud >= min && diff_ud <= snap_height;
        default:
            return false;
    }
}

void snap_player_to_slope(int obj, Player *player) {
    if (player->gamemode == GAMEMODE_PLAYER) {
        float base = RadToDeg(slope_snap_angle(obj, player));
        float bestSnap = base;
        float minDiff = 999999.0f;

        for (int i = 0; i < 4; ++i) {
            float snapAngle = base + i * 90.0f;
            
            while (snapAngle < 0) snapAngle += 360.0f;
            while (snapAngle >= 360.0f) snapAngle -= 360.0f;

            float diff = fabsf(snapAngle - (player->rotation - 0.01f));
            if (diff > 180.0f) diff = 360.0f - diff;

            if (diff < minDiff) {
                minDiff = diff;
                bestSnap = snapAngle;
            }
        }

        player->rotation = bestSnap;
    } else if (player->gamemode == GAMEMODE_BIRD) {
        player->rotation = RadToDeg(slope_snap_angle(obj, player));
    }
}
