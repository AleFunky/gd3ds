//
// For the next one that tries to fix slopes, heres a counter of hours wasted trying to fix slopes: 10 hours
//

#include "slope.h"
#include "collision.h"
#include "player.h"
#include <math.h>
#include "state.h"
#include <citro3d.h>
#include "graphics.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "menus/core/ui_screen.h"
#include "math_helpers.h"
#include "main.h"

const float falls[SPEED_COUNT] = {
    226.044054,
    280.422108,
    348.678108,
    421.200108
};

#define SLOPE_EPSILON 0.01f

// Count how many slopes is the player touching, in the case the player has an slope, it checks only the slopes with the same orientation
int get_player_touching_slopes(Player *player) {
    int sx = (int)(player->x / SECTION_SIZE);
    int sy = (int)(player->y / SECTION_SIZE);

    int count = 0;
    int slope = player->slope_data.slope_id;
    
    // Count slopes
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            Section *sec = get_section(sx + dx, sy + dy);
            for (int i = 0; i < sec->object_count; i++) {
                int obj = sec->objects[i];

                // Skip invalid objects
                if (!is_valid_object(objects.id[obj])) continue;

                const ObjectHitbox *hitbox = game_objects[objects.id[obj]].hitbox;

                if (!hitbox) continue;

                if (hitbox->collision_type == HITBOX_SOLID && hitbox->type == COLLISION_SLOPE) {
                    // If touching do stuff
                    bool colliding = intersect(
                        player->x, player->y, player->width + 1, player->height + 1, 0, 
                        objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
                    );
                    
                    if (colliding && slope_touching(obj, player)) {
                        if (slope >= 0) {
                            // Check if this would be the next slope if the next slope is higher than the current one
                            float diff = objects.y[obj] - objects.y[slope];
                            int orient = objects.orientation[slope];

                            bool would_be_next;

                            if (orient == ORIENT_NORMAL_UP || orient == ORIENT_UD_DOWN) {
                                would_be_next = diff > 0;
                            } else {
                                would_be_next = diff < 0;
                            }
                        
                            if ((orient == grav_slope_orient(obj, player) && 
                                slope_angle(slope, player) == slope_angle(obj, player) &&
                                would_be_next) ||
                                slope == obj
                            ) {
                                count++;
                            }
                        } else {
                            count++;
                        }
                    }
                }
            }
        }
    }

    return count;
}

void clear_slope_data(Player *player) {
    player->slope_data.slope_id = -1;
    player->slope_data.elapsed = 0;
    player->slope_data.snapDown = false;
}

void clear_coyote_slope_data(Player *player) {
    player->coyote_slope.slope_id = -1;
    player->coyote_slope.elapsed = -1;
    player->coyote_slope.snapDown = -1;
    player->slope_slide_coyote_time = 0;
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
    float ydist = mult * (player->height / 2) / cosf(angle);
    float pos_relative = ((float) objects.height[obj] / objects.width[obj]) * (player->x - obj_getLeft(obj));

    float y;
    // Get correct slope y depending on combination of player gravity and slope orientation
    if ((angle > 0) ^ player->upside_down ^ flipping) {
        y = obj_getBottom(obj) + MIN(pos_relative + ydist, objects.height[obj] + player->height / 2);
    } else {
        y = obj_getTop(obj) - MIN(pos_relative - ydist, objects.height[obj] + player->height / 2);
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
            
            if (player->vel_y > 0) {
                player->vel_y = 0;
            }
            break;
    }
}

#define SHIP_UFO_EXITING_VEL (508.248f / 4)

void slope_calc(int obj, Player *player) {
    int orientation = grav_slope_orient(obj, player);
    if (orientation == ORIENT_NORMAL_UP) { // Normal - up
        // Make the player start with higher velocity 
        if ((player->gamemode == GAMEMODE_BIRD || player->gamemode == GAMEMODE_SHIP) && player->vel_y < SHIP_UFO_EXITING_VEL && state.input.holdJump) {
            player->vel_y = SHIP_UFO_EXITING_VEL; // 2 in gd
        }
        
        // Handle leaving slope
        if (!slope_touching(obj, player)) {
            push_player_action(clear_slope_data);
            return;
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj))
            slope_snap_y(obj, player);

        //output_log("Tick %d - NRM player %.2f, obj %.2f slopes %d\n", player->frame, gravBottom(player), obj_gravTop(player, obj), get_player_touching_slopes(player));

        // Sliding off slope
        if (gravBottom(player) >= obj_gravTop(player, obj) - SLOPE_EPSILON && get_player_touching_slopes(player) < 2) {
            float vel = 0.9f * MIN(1.12f / slope_angle(obj, player), 1.54f) * (objects.height[obj] * player_speeds[state.speed] / objects.width[obj]);
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

            player->inverse_rotation = true;
            player->coyote_slope = player->slope_data;
            player->slope_slide_coyote_time = 2;

            player->new_vel_y = vel;// + player->gravity * STEPS_DT;
            player->jumped = true;
            
            //output_log("Tick %d - Time %.2f PElapsed %.4f SElapsed %.4f Exit vel %.2f\n", player->frame, time, player->timeElapsed, player->slope_data.elapsed, player->new_vel_y);
            
            push_player_action(clear_slope_data);
        }
    } else if (orientation == ORIENT_NORMAL_DOWN) { // Normal - down
        // Handle leaving slope
        if (player->vel_y > 0) {
            push_player_action(clear_slope_data);
            return;
        }

        if (gravBottom(player) != obj_gravTop(player, obj) || player->slope_data.snapDown) {
            slope_snap_y(obj, player);
        }

        if ((obj_gravTop(player, obj) <= grav(player, player->y) + SLOPE_EPSILON || getLeft(player) - obj_getRight(obj) > 0) && get_player_touching_slopes(player) < 2) {
            float vel = -falls[state.speed] * ((float) objects.height[obj] / objects.width[obj]);
            player->new_vel_y = vel;
            player->coyote_slope = player->slope_data;
            player->slope_slide_coyote_time = 2;
            push_player_action(clear_slope_data);
        }        
    } else if (orientation == ORIENT_UD_UP) { // Upside down - up
        // Make the player start with higher velocity 
        if ((player->gamemode == GAMEMODE_BIRD || player->gamemode == GAMEMODE_SHIP) && player->vel_y > -SHIP_UFO_EXITING_VEL && !state.input.holdJump) {
            player->vel_y = -SHIP_UFO_EXITING_VEL; // 2 in gd
        }

        // Handle leaving slope
        if (!slope_touching(obj, player)) {
            push_player_action(clear_slope_data);
            return;
        }
        
        bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj_id >= 0 && GET_HITBOX_COUNTER(player->gravObj_id) == 1);
        
        if (player->gamemode == GAMEMODE_PLAYER && !gravSnap) {
            kill_player(DEATH_SLOPE);
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj))
            slope_snap_y(obj, player);

        //output_log("Tick %d - UD player %f, obj %f slopes %d\n", player->frame, gravTop(player), obj_gravBottom(player, obj), get_player_touching_slopes(player));

        // Sliding off slope
        if (gravTop(player) <= obj_gravBottom(player, obj) + SLOPE_EPSILON && get_player_touching_slopes(player) < 2) {
            //output_log("Tick %d - player %.2f, obj %.2f slopes %d\n", player->frame, gravTop(player), obj_gravBottom(player, obj), get_player_touching_slopes(player));
            float vel = 0.9f * MIN(1.12f / slope_angle(obj, player), 1.54f) * (objects.height[obj] * player_speeds[state.speed] / objects.width[obj]);
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
            
            player->inverse_rotation = true;
            player->coyote_slope = player->slope_data;
            player->slope_slide_coyote_time = 2;
            push_player_action(clear_slope_data);
            
            player->new_vel_y = -vel;

            //output_log("Tick %d - Time %.2f PElapsed %.2f SElapsed %.2f Exit vel %.2f\n", player->frame, time, player->timeElapsed, player->slope_data.elapsed, player->new_vel_y);
        }
    } else if (orientation == ORIENT_UD_DOWN) { // Upside down - down
        // Handle leaving slope
        if (player->vel_y < 0) {
            push_player_action(clear_slope_data);
            return;
        }
        
        bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj_id >= 0 && GET_HITBOX_COUNTER(player->gravObj_id) == 1);
        
        if (player->gamemode == GAMEMODE_PLAYER && !gravSnap) {
            kill_player(DEATH_SLOPE);
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj) || player->slope_data.snapDown) {
            slope_snap_y(obj, player);
        }

        // Sliding off
        if ((obj_gravTop(player, obj) <= grav(player, player->y) - SLOPE_EPSILON || getLeft(player) - obj_getRight(obj) > 0) && get_player_touching_slopes(player) < 2) {
            float vel = falls[state.speed] * ((float) objects.height[obj] / objects.width[obj]);
            player->new_vel_y = vel;
            player->coyote_slope = player->slope_data;
            player->slope_slide_coyote_time = 2;
            push_player_action(clear_slope_data);
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

    bool hasSlope = state.old_player.slope_data.slope_id >= 0;

    if (objects.orientation[obj] < 2 && expected_slope_y(obj, player) + (hasSlope ? 3 : 0) <= player->y)
		return;
	else if (objects.orientation[obj] >= 2 && expected_slope_y(obj, player) - (hasSlope ? 3 : 0) >= player->y)
		return;
    
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
        if (internalCollidingSlope) kill_player(DEATH_SLOPE);
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

            if (internalCollidingSlope) kill_player(DEATH_SLOPE);
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

            if (internalCollidingSlope) kill_player(DEATH_SLOPE);
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

            if (internalCollidingSlope) kill_player(DEATH_SLOPE);
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
    //output_log("Tick %d - dir %d oldDir %d touching %d colliding %d \n", player->frame, orient, (slope >= 0 ? grav_slope_orient(slope, player) : -1), slope_touching(obj, player), colliding);

    // Check if this would be the next slope if the next slope is higher than the current one
    
    
    bool would_be_next = true;
    if (slope >= 0) {
        float diff = objects.y[obj] - objects.y[slope];
        int orient = objects.orientation[slope];


        if (orient == ORIENT_NORMAL_UP || orient == ORIENT_UD_DOWN) {
            would_be_next = diff > 0;
        } else {
            would_be_next = diff < 0;
        }
    }
    
    // Check if going from going down to up
    bool up_to_down = slope >= 0 && grav_slope_orient(slope, player) != grav_slope_orient(obj, player) && 
    (
        (grav_slope_orient(slope, player) == ORIENT_NORMAL_DOWN && grav_slope_orient(obj, player) == ORIENT_NORMAL_UP) ||
        (grav_slope_orient(slope, player) == ORIENT_UD_DOWN && grav_slope_orient(obj, player) == ORIENT_UD_UP)
    );

    if ((slope < 0 || grav_slope_orient(slope, player) == grav_slope_orient(obj, player) || up_to_down) && slope_touching(obj, player) && colliding && obj_gravTop(player, obj) - gravBottom(player) > 2 && (up_to_down || would_be_next)
    ) {
        if (slope >= 0 && slope_angle(obj, player) < slope_angle(slope, player)) return;
        
        int oldOrient = grav_slope_orient(obj, &state.old_player);
        float oldVel = (oldOrient == ORIENT_NORMAL_DOWN || oldOrient == ORIENT_UD_DOWN) ? grav(player, state.old_player.delta_y) : (state.old_player.vel_y * STEPS_DT);

        float angle = atanf(oldVel / (player_speeds[state.speed] * STEPS_DT));
        if (oldOrient >= ORIENT_UD_DOWN) angle = -angle;

        bool hasSlope = state.old_player.slope_data.slope_id >= 0;

        // Check if the old slope and this slope have the same orientation, if not, then the player doesn't have an slope
        if (hasSlope && slope >= 0) {
            hasSlope = objects.orientation[state.old_player.slope_data.slope_id] == objects.orientation[slope];
        }

        bool projectedHit = (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) ? (angle * 5.f <= slope_angle(obj, player)) : (angle <= slope_angle(obj, player));
        bool clip = true;//slope_touching(obj, player);
        bool snapDown = (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) && player->vel_y * mult > 0 && player->x - obj_getLeft(obj) > 0;

        //output_log("Tick %d - dir %d oldDir %d proyectedHit %d clip %d snapDown %d angle %f slope_angle %f\n", player->frame, orient, (slope >= 0 ? grav_slope_orient(slope, player) : -1), projectedHit, clip, snapDown, angle, slope_angle(obj, player));

        if ((projectedHit && clip) || snapDown) {
            // If wave, just die, nothing else, wave hates slopes
            if (player->gamemode == GAMEMODE_DART) {
                kill_player(DEATH_SLOPE);
                return;
            }

            if (orient >= ORIENT_UD_DOWN) player->on_ceiling = true;
            else {
                player->on_ground = true;
                player_non_flying_landing(player);
            }
            
            player->inverse_rotation = false;
            player->slope_data.slope_id = obj;
            //slope_snap_y(obj, player);

            if (is_spike_slope(obj)) {
                kill_player(DEATH_SLOPE);
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

            //if (state.old_player.slope_data.slope_id < 0) {
            //    player->slope_data.elapsed = 0.f;
            //}

            // If the player wasn't on an slope, initialize the time elapsed
            if (!player->slope_data.elapsed) {
                Player *other_player = (state.current_player == 0 ? &state.player2 : &state.player);

                // Make both times synced if close enough
                if (state.dual && other_player->slope_data.slope_id >= 0 && fabsf(player->timeElapsed - other_player->slope_data.elapsed) < 0.10) {
                    player->slope_data.elapsed = other_player->slope_data.elapsed;
                } else {
                    player->slope_data.elapsed = state.old_player.timeElapsed;
                }
            }
            //output_log("elapsed %.2f\n", player->slope_data.elapsed);

            slope_calc(obj, player);
        } 
    }
}

bool slope_touching(int obj, Player *player) {
    bool hasSlope = state.old_player.slope_data.slope_id >= 0;

    // If player is on slope, add a bit of hitbox
    if (hasSlope) {
        int mult = grav_slope_orient(state.old_player.slope_data.slope_id, player) >= ORIENT_UD_DOWN ? -1 : 1;
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