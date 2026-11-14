//
// For the next one that tries to fix slopes, heres a counter of hours wasted trying to fix slopes: 10 hours
//

#include "slope.hpp"
#include "collision.hpp"
#include "player.hpp"
#include <math.h>
#include "state.hpp"
#include <citro3d.h>
#include "graphics.hpp"
#include "text.hpp"
#include "fonts/bigFont.hpp"
#include "menus/core/ui_screen.hpp"
#include "math_helpers.hpp"
#include "main.hpp"

namespace gd3ds
{

const float falls[static_cast<int>(PlayerSpeeds::SpeedCount)] = {
    226.044054,
    280.422108,
    348.678108,
    421.200108,
    518.400173
};

#define SLOPE_EPSILON 0.01f

// Count how many slopes is the player touching, in the case the player has an slope, it checks only the slopes with the same orientation
int getPlayerTouchingSlopes(Player *player) {
    int sx = (int)(player->x / SECTION_SIZE);
    int sy = (int)(player->y / SECTION_SIZE);

    int count = 0;
    int slope = player->slopeData.slopeID;
    
    // Count slopes
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            Section *sec = getSection(sx + dx, sy + dy);
            for (int i = 0; i < sec->objectCount; i++) {
                int obj = sec->objects[i];

                // Skip invalid objects
                if (!isValidObject(objects.id[obj])) continue;

                const ObjectHitbox *hitbox = gameObjects[objects.id[obj]].hitbox;

                if (!hitbox) continue;

                if (hitbox->collisionType == static_cast<int>(HitboxType::HitboxSolid) && hitbox->type == static_cast<int>(CollisionShape::CollisionSlope)) {
                    // If touching do stuff
                    bool colliding = intersect(
                        player->x, player->y, player->width + 1, player->height + 1, 0, 
                        objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
                    );
                    
                    if (colliding && slopeTouching(obj, player)) {
                        if (slope >= 0) {
                            // Check if this would be the next slope if the next slope is higher than the current one
                            float diff = objects.y[obj] - objects.y[slope];
                            int orient = objects.orientation[slope];

                            bool wouldBeNext;

                            if (orient == static_cast<int>(SlopeOrientations::OrientNormalUp) || orient == static_cast<int>(SlopeOrientations::OrientUdDown)) {
                                wouldBeNext = diff > 0;
                            } else {
                                wouldBeNext = diff < 0;
                            }
                        
                            if ((orient == gravSlopeOrient(obj, player) && 
                                slopeAngle(slope, player) == slopeAngle(obj, player) &&
                                wouldBeNext) ||
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

void clearSlopeData(Player *player) {
    player->slopeData.slopeID = -1;
    player->slopeData.elapsed = 0;
    player->slopeData.snapDown = false;
}

void clearCoyoteSlopeData(Player *player) {
    player->coyoteSlope.slopeID = -1;
    player->coyoteSlope.elapsed = -1;
    player->coyoteSlope.snapDown = -1;
    player->slopeSlideCoyoteTime = 0;
}

int gravSlopeOrient(int obj, Player *player) {
    int orient = objects.orientation[obj];

    if (player->upsideDown) {
        // Flip vertically slope orientation
        if (orient == static_cast<int>(SlopeOrientations::OrientUdUp))
			orient = static_cast<int>(SlopeOrientations::OrientNormalUp);
		else if (orient == static_cast<int>(SlopeOrientations::OrientUdDown))
			orient = static_cast<int>(SlopeOrientations::OrientNormalDown);
		else if (orient == static_cast<int>(SlopeOrientations::OrientNormalUp))
			orient = static_cast<int>(SlopeOrientations::OrientUdUp);
		else if (orient == static_cast<int>(SlopeOrientations::OrientNormalDown))
			orient = static_cast<int>(SlopeOrientations::OrientUdDown);
    }
    return orient;
}

bool isSpikeSlope(int obj) {
    switch (objects.id[obj]) {
        case 363:
        case 364:
        case 366:
        case 367:
        case 1717:
        case 1718:
        case 1723:
        case 1724:
            return true;
    }
    return false;
}

float slopeAngle(int obj, Player *player) {
    float angle = atanf((float) objects.height[obj] / objects.width[obj]);
    int orient = gravSlopeOrient(obj, player);
    if (orient == static_cast<int>(SlopeOrientations::OrientNormalDown) || orient == static_cast<int>(SlopeOrientations::OrientUdDown)) {
        angle = -angle;
    }

    return angle;
}

float getSlopeAngle(int obj) {
    float angle = atanf((float) objects.height[obj] / objects.width[obj]);
    return angle;
}

float slopeSnapAngle(int obj, Player *player) {
    float angle = slopeAngle(obj, player);
    int orient = objects.orientation[obj];

    if (orient == static_cast<int>(SlopeOrientations::OrientNormalUp)) angle = -fabsf(angle);
    if (orient == static_cast<int>(SlopeOrientations::OrientNormalDown)) angle = fabsf(angle);

    return angle;
}

float expectedSlopeY(int obj, Player *player) {
    int flipping = gravSlopeOrient(obj, player) >= static_cast<int>(SlopeOrientations::OrientUdDown);
    int mult = (player->upsideDown ^ flipping) ? -1 : 1;
    
    float angle = slopeAngle(obj, player);
    float ydist = mult * (player->height / 2) / cosf(angle);
    float posRelative = ((float) objects.height[obj] / objects.width[obj]) * (player->x - objGetLeft(obj));

    float y;
    // Get correct slope y depending on combination of player gravity and slope orientation
    if ((angle > 0) ^ player->upsideDown ^ flipping) {
        y = objGetBottom(obj) + MIN(posRelative + ydist, objects.height[obj] + player->height / 2);
    } else {
        y = objGetTop(obj) - MIN(posRelative - ydist, objects.height[obj] + player->height / 2);
    }

    // Spike slope has bigger hitbox
    if (isSpikeSlope(obj)) {
        y += (objects.orientation[obj] >= static_cast<int>(SlopeOrientations::OrientUdDown) ? -4 : 4);
    }

    return y;

}

void slopeSnapY(int obj, Player *player) {
    int orientation = gravSlopeOrient(obj, player);

    switch (orientation) {
        case static_cast<int>(SlopeOrientations::OrientNormalUp): // Normal - up
            if (player->upsideDown) {
                player->y = MAX(objGetBottom(obj) - player->height / 2, expectedSlopeY(obj, player));
            } else {
                player->y = MIN(objGetTop(obj) + player->height / 2, expectedSlopeY(obj, player));
            }

            player->timeSinceGround = 0;
            player->onGround = true;
            
            if (player->velY < 0) {
                player->velY = 0;
            }
            break;
        case static_cast<int>(SlopeOrientations::OrientNormalDown): // Normal - down
            if (player->upsideDown) {
                player->y = MIN(expectedSlopeY(obj, player), objGetTop(obj) + player->height / 2);
            } else {
                player->y = MAX(expectedSlopeY(obj, player), objGetBottom(obj) - player->height / 2);
            }
            
            player->timeSinceGround = 0;
            player->onGround = true;
            
            if (player->velY < 0) {
                player->velY = 0;
            }
            break;
        case static_cast<int>(SlopeOrientations::OrientUdDown): // Upside down - down
            if (player->upsideDown) {
                player->y = MAX(expectedSlopeY(obj, player), objGetBottom(obj) - player->height / 2);
            } else {
                player->y = MIN(expectedSlopeY(obj, player), objGetTop(obj) + player->height / 2);
            }
            
            player->timeSinceGround = 0;
            player->onCeiling = true;

            if (player->velY > 0) {
                player->velY = 0;
            }
            break;
        case static_cast<int>(SlopeOrientations::OrientUdUp): // Upside down - up
            if (player->upsideDown) {
                player->y = MIN(objGetTop(obj) + player->height / 2, expectedSlopeY(obj, player));
            } else {
                player->y = MAX(objGetBottom(obj) - player->height / 2, expectedSlopeY(obj, player));
            }

            player->timeSinceGround = 0;
            player->onCeiling = true;
            
            if (player->velY > 0) {
                player->velY = 0;
            }
            break;
    }
}

#define SHIP_UFO_EXITING_VEL (508.248f / 4)

static float slopeLaunchVelocity(int obj, Player *player) {
    float vel = 0.9f * MIN(1.12f / slopeAngle(obj, player), 1.54f) * (objects.height[obj] * playerSpeeds[state.speed] / objects.width[obj]);
    float time = clampf(10 * (player->timeElapsed - player->slopeData.elapsed), 0.4f, 1.0f);

    if (player->gamemode == static_cast<int>(IconGamemode::GamemodeBall)) vel *= 0.75f;
    if (player->gamemode == static_cast<int>(IconGamemode::GamemodeShip)) vel *= 0.75f;
    if (player->gamemode == static_cast<int>(IconGamemode::GamemodeUfo)) vel *= 0.7499f;

    return vel * time;
}

void slopeCalc(int obj, Player *player) {
    int orientation = gravSlopeOrient(obj, player);
    if (orientation == static_cast<int>(SlopeOrientations::OrientNormalUp)) { // Normal - up
        // Make the player start with higher velocity 
        if ((player->gamemode == static_cast<int>(IconGamemode::GamemodeUfo) || player->gamemode == static_cast<int>(IconGamemode::GamemodeShip)) && player->velY < SHIP_UFO_EXITING_VEL && currInput.holdJump) {
            player->velY = SHIP_UFO_EXITING_VEL; // 2 in gd
        }
        
        // Handle leaving slope
        if (!slopeTouching(obj, player)) {
            if (player->gamemode == static_cast<int>(IconGamemode::GamemodeShip) || player->gamemode == static_cast<int>(IconGamemode::GamemodeUfo)) {
                float launch = slopeLaunchVelocity(obj, player);
                if (player->velY < launch) player->newVelY = launch;
            }

            pushPlayerAction(clearSlopeData);
            return;
        }

        // On slope
        if (gravBottom(player) != objGravTop(player, obj))
            slopeSnapY(obj, player);

        // Sliding off slope
        if (gravBottom(player) >= objGravTop(player, obj) - SLOPE_EPSILON && getPlayerTouchingSlopes(player) < 2) {
            float vel = slopeLaunchVelocity(obj, player);

            player->inverseRotation = true;
            player->coyoteSlope = player->slopeData;
            player->slopeSlideCoyoteTime = 2;

            if (player->velY < vel) player->newVelY = vel;
            player->jumped = true;
            
            //output_log("Tick %d - Time %.2f PElapsed %.4f SElapsed %.4f Exit vel %.2f\n", player->frame, time, player->timeElapsed, player->slope_data.elapsed, player->new_vel_y);
            
            pushPlayerAction(clearSlopeData);
        }
    } else if (orientation == static_cast<int>(SlopeOrientations::OrientNormalDown)) { // Normal - down
        // Handle leaving slope
        if (player->velY > 0) {
            pushPlayerAction(clearSlopeData);
            return;
        }

        if (gravBottom(player) != objGravTop(player, obj) || player->slopeData.snapDown) {
            slopeSnapY(obj, player);
        }

        if ((objGravTop(player, obj) <= grav(player, player->y) + SLOPE_EPSILON || getLeft(player) - objGetRight(obj) > 0) && getPlayerTouchingSlopes(player) < 2) {
            float vel = -falls[state.speed] * ((float) objects.height[obj] / objects.width[obj]);
            player->newVelY = vel;
            player->coyoteSlope = player->slopeData;
            player->slopeSlideCoyoteTime = 2;
            pushPlayerAction(clearSlopeData);
        }        
    } else if (orientation == static_cast<int>(SlopeOrientations::OrientUdUp)) { // Upside down - up
        // Make the player start with higher velocity 
        if ((player->gamemode == static_cast<int>(IconGamemode::GamemodeUfo) || player->gamemode == static_cast<int>(IconGamemode::GamemodeShip)) && player->velY > -SHIP_UFO_EXITING_VEL && !currInput.holdJump) {
            player->velY = -SHIP_UFO_EXITING_VEL; // 2 in gd
        }

        // Handle leaving slope
        if (!slopeTouching(obj, player)) {
            pushPlayerAction(clearSlopeData);
            return;
        }
        
        bool gravSnap = (player->ceilingInvTime > 0) || (player->gravObjID >= 0 && GET_HITBOX_COUNTER(player->gravObjID) == 1);
        
        if (player->gamemode == static_cast<int>(IconGamemode::GamemodePlayer) && !gravSnap && !player->allowHeadHit) {
            if (grav(player, player->y - SLOPE_HEAD_TOLERANCE) > grav(player, expectedSlopeY(obj, player))) {
                killPlayer(DeathReason::DeathSlope);
            }
            return;
        }

        // On slope
        if (gravBottom(player) != objGravTop(player, obj))
            slopeSnapY(obj, player);

        //output_log("Tick %d - UD player %f, obj %f slopes %d\n", player->frame, gravTop(player), obj_gravBottom(player, obj), get_player_touching_slopes(player));

        // Sliding off slope
        if (gravTop(player) <= objGravBottom(player, obj) + SLOPE_EPSILON && getPlayerTouchingSlopes(player) < 2) {
            //output_log("Tick %d - player %.2f, obj %.2f slopes %d\n", player->frame, gravTop(player), obj_gravBottom(player, obj), get_player_touching_slopes(player));
            float vel = 0.9f * MIN(1.12f / slopeAngle(obj, player), 1.54f) * (objects.height[obj] * playerSpeeds[state.speed] / objects.width[obj]);
            float time = clampf(10 * (player->timeElapsed - player->slopeData.elapsed), 0.4f, 1.0f);
            
            //float orig = vel;
            if (player->gamemode == static_cast<int>(IconGamemode::GamemodeBall)) {
                vel *= 0.75f;
            }
            
            if (player->gamemode == static_cast<int>(IconGamemode::GamemodeShip)) {
                vel *= 0.75f;
            }

            if (player->gamemode == static_cast<int>(IconGamemode::GamemodeUfo)) {
                vel *= 0.7499f;
            }

            vel *= time;
            
            player->inverseRotation = true;
            player->coyoteSlope = player->slopeData;
            player->slopeSlideCoyoteTime = 2;
            pushPlayerAction(clearSlopeData);
            
            player->newVelY = -vel;

            //output_log("Tick %d - Time %.2f PElapsed %.2f SElapsed %.2f Exit vel %.2f\n", player->frame, time, player->timeElapsed, player->slope_data.elapsed, player->new_vel_y);
        }
    } else if (orientation == static_cast<int>(SlopeOrientations::OrientUdDown)) { // Upside down - down
        // Handle leaving slope
        if (player->velY < 0) {
            pushPlayerAction(clearSlopeData);
            return;
        }
        
        bool gravSnap = (player->ceilingInvTime > 0) || (player->gravObjID >= 0 && GET_HITBOX_COUNTER(player->gravObjID) == 1);
        
        if (player->gamemode == static_cast<int>(IconGamemode::GamemodePlayer) && !gravSnap && !player->allowHeadHit) {
            if (grav(player, player->y - SLOPE_HEAD_TOLERANCE) > grav(player, expectedSlopeY(obj, player))) {
                killPlayer(DeathReason::DeathSlope);
            }
            return;
        }

        // On slope
        if (gravBottom(player) != objGravTop(player, obj) || player->slopeData.snapDown) {
            slopeSnapY(obj, player);
        }

        // Sliding off
        if ((objGravTop(player, obj) <= grav(player, player->y) - SLOPE_EPSILON || getLeft(player) - objGetRight(obj) > 0) && getPlayerTouchingSlopes(player) < 2) {
            float vel = falls[state.speed] * ((float) objects.height[obj] / objects.width[obj]);
            player->newVelY = vel;
            player->coyoteSlope = player->slopeData;
            player->slopeSlideCoyoteTime = 2;
            pushPlayerAction(clearSlopeData);
        }
    }
}


bool playerCircleTouchesSlope(int obj, Player *player) {
    float x1, y1, x2, y2;
    int orientation = objects.orientation[obj];

    float hw = objects.width[obj] / 2.f, hh = objects.height[obj] / 2.f;

    float playerRadius = (player->width - 4) / 2;

    // Collide with hipotenuse
    switch (orientation) {
        case static_cast<int>(SlopeOrientations::OrientNormalUp):
        case static_cast<int>(SlopeOrientations::OrientUdDown): {
            x1 = objects.x[obj] - hw;
            y1 = objects.y[obj] - hh;
            x2 = objects.x[obj] + hw;
            y2 = objects.y[obj] + hh;
            break;
        }
        case static_cast<int>(SlopeOrientations::OrientNormalDown):
        case static_cast<int>(SlopeOrientations::OrientUdUp): {
            x1 = objects.x[obj] + hw;
            y1 = objects.y[obj] - hh;
            x2 = objects.x[obj] - hw;
            y2 = objects.y[obj] + hh;
            break;
        }
        default: {
            x1 = y1 = x2 = y2 = 0;
            break;
        }
    }
    bool collidedHipo = circleRectCollision(player->x, player->y, playerRadius, x1, y1, x2, y2);

    // Collide with vertical
    switch (orientation) {
        case static_cast<int>(SlopeOrientations::OrientNormalUp):
        case static_cast<int>(SlopeOrientations::OrientUdUp):
            x1 = objects.x[obj] + hw;
            y1 = objects.y[obj] - hh;
            x2 = objects.x[obj] + hw;
            y2 = objects.y[obj]  + hh;
            break;
        case static_cast<int>(SlopeOrientations::OrientNormalDown):
        case static_cast<int>(SlopeOrientations::OrientUdDown):
            x1 = objects.x[obj] - hw;
            y1 = objects.y[obj]  - hh;
            x2 = objects.x[obj] - hw;
            y2 = objects.y[obj]  + hh;
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
            break;
    }
    
    bool collidedVertical = circleRectCollision(player->x, player->y, playerRadius, x1, y1, x2, y2);

    // Collide with horizontal
    switch (orientation) {
        case static_cast<int>(SlopeOrientations::OrientNormalUp):
        case static_cast<int>(SlopeOrientations::OrientNormalDown):
            x1 = objects.x[obj] + hw;
            y1 = objects.y[obj]  - hh;
            x2 = objects.x[obj] - hw;
            y2 = objects.y[obj]  - hh;
            break;
        case static_cast<int>(SlopeOrientations::OrientUdDown):
        case static_cast<int>(SlopeOrientations::OrientUdUp):
            x1 = objects.x[obj] + hw;
            y1 = objects.y[obj]  + hh;
            x2 = objects.x[obj] - hw;
            y2 = objects.y[obj]  + hh;
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
            break;
    }
    
    bool collidedHorizontal = circleRectCollision(player->x, player->y, playerRadius, x1, y1, x2, y2);

    return collidedVertical | collidedHipo | collidedHorizontal;
}

void slopeCollide(int obj, Player *player) {
    if (potentialSlopes[state.currentPlayer] < MAX_COLLIDED_OBJECTS) {
        potentialSlopesBuffer[state.currentPlayer][potentialSlopes[state.currentPlayer]++] = obj;
    }

    bool hasSlope = state.oldPlayer.slopeData.slopeID >= 0;

    if (objects.orientation[obj] < 2 && expectedSlopeY(obj, player) + (hasSlope ? 3 : 0) <= player->y)
		return;
	else if (objects.orientation[obj] >= 2 && expectedSlopeY(obj, player) - (hasSlope ? 3 : 0) >= player->y)
		return;
    
    int clip = (player->gamemode == static_cast<int>(IconGamemode::GamemodeShip) || player->gamemode == static_cast<int>(IconGamemode::GamemodeUfo)) ? 7 : 10;
    int orient = gravSlopeOrient(obj, player);  
    int mult = orient >= static_cast<int>(SlopeOrientations::OrientUdDown) ? -1 : 1;

    InternalHitbox internal = player->internalHitbox;

    bool gravSnap = (player->ceilingInvTime > 0) || (player->gravObjID >= 0 && GET_HITBOX_COUNTER(player->gravObjID) == 1);

    // Check if player inside slope
    if (orient == static_cast<int>(SlopeOrientations::OrientNormalUp) || orient == static_cast<int>(SlopeOrientations::OrientUdUp)) {
        bool internalCollidingSlope = intersect(
            player->x, player->y, internal.width, internal.height, 0, 
            objGetRight(obj), objects.y[obj] , 1, objects.height[obj], 0
        );

        // Die if so
        if (internalCollidingSlope) killPlayer(DeathReason::DeathSlope);
    }

    // Normal slope - resting on bottom
    if (
        !(player->gamemode == static_cast<int>(IconGamemode::GamemodeWave) && player->allowDartSlide) &&
        state.oldPlayer.slopeData.slopeID < 0 &&
        orient < static_cast<int>(SlopeOrientations::OrientUdDown) &&
        gravTop(player) - objGravBottom(player, obj) <= clip + 5 * !player->mini // Remove extra if mini
    ) {
        if ((player->gamemode != static_cast<int>(IconGamemode::GamemodeWave) || player->allowDartSlide) && ((player->gamemode != static_cast<int>(IconGamemode::GamemodePlayer) && (player->velY >= 0)) || gravSnap)) {
            player->velY = 0;
            player->collidedBlock = obj;
            if (!gravSnap) player->onCeiling = true;
            player->timeSinceGround = 0;
            player->y = grav(player, objGravBottom(player, obj)) - grav(player, player->height / 2);
        } else {
            bool internalCollidingSlope = intersectExclusive(
                player->x, player->y, internal.width, internal.height, 0, 
                objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
            );

            if (internalCollidingSlope) killPlayer(DeathReason::DeathSlope);
        }

        return;
    }

    // Upside down slope - resting on top
    if (
        !(player->gamemode == static_cast<int>(IconGamemode::GamemodeWave) && player->allowDartSlide) &&
        state.oldPlayer.slopeData.slopeID < 0 &&
        orient >= static_cast<int>(SlopeOrientations::OrientUdDown) &&
        objGravTop(player, obj) - gravBottom(player) <= clip + 5 * !player->mini // Remove extra if mini
    ) {
        if ((player->gamemode != static_cast<int>(IconGamemode::GamemodeWave) || player->allowDartSlide) && player->velY <= 0) {
            player->velY = 0;
            if (!gravSnap) player->onGround = true;
            player->timeSinceGround = 0;
            player->y = grav(player, objGravTop(player, obj)) + grav(player, player->height / 2);
        } else {
            bool internalCollidingSlope = intersectExclusive(
                player->x, player->y, internal.width, internal.height, 0, 
                objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
            );

            if (internalCollidingSlope) killPlayer(DeathReason::DeathSlope);
        }
        
        return;
    }

    // Left side collision
    if (
        state.oldPlayer.slopeData.slopeID < 0 && 
        (orient == static_cast<int>(SlopeOrientations::OrientNormalDown) || orient == static_cast<int>(SlopeOrientations::OrientUdDown)) &&
        player->x - objGetLeft(obj) < 0
    ) {
        // Going from the left
        if (objGravTop(player, obj) - gravBottom(player) > clip) {
            bool internalCollidingSlope = intersectExclusive(
                player->x, player->y, internal.width, internal.height, 0, 
                objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
            );

            if (internalCollidingSlope) killPlayer(DeathReason::DeathSlope);
            return;
        }
        
        // Touching slope before center is in slope
        if ((player->gamemode != static_cast<int>(IconGamemode::GamemodeWave) || player->allowDartSlide) && player->velY * mult <= 0) {
            if (orient == static_cast<int>(SlopeOrientations::OrientNormalDown)) {
                player->y = grav(player, objGravTop(player, obj)) + grav(player, player->height / 2);
            } else {
                player->y = grav(player, objGravBottom(player, obj)) - grav(player, player->height / 2);
            }
            player->onGround = true;
            player->inverseRotation = false;
            return;
        }
    }

    if (!gravSnap && player->gamemode == static_cast<int>(IconGamemode::GamemodePlayer) && gravSlopeOrient(obj, player) >= 2 && !playerCircleTouchesSlope(obj, player)) return;

    bool colliding = intersectExclusive(
        player->x, player->y, player->width, player->height, 0, 
        objects.x[obj], objects.y[obj], objects.width[obj], objects.height[obj], 0
    );

    int slope = player->slopeData.slopeID;
    //output_log("Tick %d - dir %d oldDir %d touching %d colliding %d \n", player->frame, orient, (slope >= 0 ? grav_slope_orient(slope, player) : -1), slope_touching(obj, player), colliding);

    // Check if this would be the next slope if the next slope is higher than the current one
    
    
    bool wouldBeNext = true;
    if (slope >= 0) {
        float diff = objects.y[obj] - objects.y[slope];
        int orient = objects.orientation[slope];


        if (orient == static_cast<int>(SlopeOrientations::OrientNormalUp) || orient == static_cast<int>(SlopeOrientations::OrientUdDown)) {
            wouldBeNext = diff > 0;
        } else {
            wouldBeNext = diff < 0;
        }
    }
    
    // Check if going from going down to up
    bool upToDown = slope >= 0 && gravSlopeOrient(slope, player) != gravSlopeOrient(obj, player) && 
    (
        (gravSlopeOrient(slope, player) == static_cast<int>(SlopeOrientations::OrientNormalDown) && gravSlopeOrient(obj, player) == static_cast<int>(SlopeOrientations::OrientNormalUp)) ||
        (gravSlopeOrient(slope, player) == static_cast<int>(SlopeOrientations::OrientUdDown) && gravSlopeOrient(obj, player) == static_cast<int>(SlopeOrientations::OrientUdUp))
    );

    if ((slope < 0 || gravSlopeOrient(slope, player) == gravSlopeOrient(obj, player) || upToDown) && slopeTouching(obj, player) && colliding && objGravTop(player, obj) - gravBottom(player) > 2 && (upToDown || wouldBeNext)
    ) {
        if (slope >= 0 && slopeAngle(obj, player) < slopeAngle(slope, player)) return;
        
        int oldOrient = gravSlopeOrient(obj, &state.oldPlayer);
        float oldVel = (oldOrient == static_cast<int>(SlopeOrientations::OrientNormalDown) || oldOrient == static_cast<int>(SlopeOrientations::OrientUdDown)) ? grav(player, state.oldPlayer.deltaY) : (state.oldPlayer.velY * STEPS_DT);

        float angle = atanf(oldVel / (playerSpeeds[state.speed] * STEPS_DT));
        if (oldOrient >= static_cast<int>(SlopeOrientations::OrientUdDown)) angle = -angle;

        bool hasSlope = state.oldPlayer.slopeData.slopeID >= 0;

        // Check if the old slope and this slope have the same orientation, if not, then the player doesn't have an slope
        if (hasSlope && slope >= 0) {
            hasSlope = objects.orientation[state.oldPlayer.slopeData.slopeID] == objects.orientation[slope];
        }

        bool projectedHit = (orient == static_cast<int>(SlopeOrientations::OrientNormalDown) || orient == static_cast<int>(SlopeOrientations::OrientUdDown)) ? (angle * 5.f <= slopeAngle(obj, player)) : (angle <= slopeAngle(obj, player));
        bool clip = true;//slope_touching(obj, player);
        bool snapDown = (orient == static_cast<int>(SlopeOrientations::OrientNormalDown) || orient == static_cast<int>(SlopeOrientations::OrientUdDown)) && player->velY * mult > 0 && player->x - objGetLeft(obj) > 0;

        //output_log("Tick %d - dir %d oldDir %d proyectedHit %d clip %d snapDown %d angle %f slope_angle %f\n", player->frame, orient, (slope >= 0 ? grav_slope_orient(slope, player) : -1), projectedHit, clip, snapDown, angle, slope_angle(obj, player));

        if ((projectedHit && clip) || snapDown) {
            // If wave, just die, nothing else, wave hates slopes
            if (player->gamemode == static_cast<int>(IconGamemode::GamemodeWave) && !player->allowDartSlide) {
                killPlayer(DeathReason::DeathSlope);
                return;
            }

            if (orient >= static_cast<int>(SlopeOrientations::OrientUdDown)) player->onCeiling = true;
            else {
                player->collidedBlock = obj;
                player->onGround = true;
                playerNonFlyingLanding(player);
            }
            
            player->inverseRotation = false;
            player->slopeData.slopeID = obj;
            //slope_snap_y(obj, player);

            if (isSpikeSlope(obj)) {
                killPlayer(DeathReason::DeathSlope);
            }

            // If player is on an slope that goes down, and is in the top corner, snap down
            if (snapDown && !hasSlope) {
                if (orient == static_cast<int>(SlopeOrientations::OrientNormalDown) && player->velY <= 0) {
                    player->y = grav(player, objGravTop(player, obj)) + grav(player, player->height / 2);
                    player->slopeData.snapDown = true;
                    player->velY = 0;
                } else if (orient == static_cast<int>(SlopeOrientations::OrientUdDown) && player->velY >= 0) {
                    player->y = grav(player, objGravBottom(player, obj)) - grav(player, player->height / 2);
                    player->slopeData.snapDown = true;
                    player->velY = 0;
                }
            }

            //if (state.old_player.slope_data.slope_id < 0) {
            //    player->slope_data.elapsed = 0.f;
            //}

            // If the player wasn't on an slope, initialize the time elapsed
            if (!player->slopeData.elapsed) {
                Player *otherPlayer = (state.currentPlayer == 0 ? &state.player2 : &state.player);

                // Make both times synced if close enough
                if (state.dual && otherPlayer->slopeData.slopeID >= 0 && fabsf(player->timeElapsed - otherPlayer->slopeData.elapsed) < 0.10) {
                    player->slopeData.elapsed = otherPlayer->slopeData.elapsed;
                } else {
                    player->slopeData.elapsed = state.oldPlayer.timeElapsed;
                }
            }
            //output_log("elapsed %.2f\n", player->slope_data.elapsed);

            slopeCalc(obj, player);
        } 
    }
}

bool slopeTouching(int obj, Player *player) {
    bool hasSlope = state.oldPlayer.slopeData.slopeID >= 0;

    // If player is on slope, add a bit of hitbox
    if (hasSlope) {
        int mult = gravSlopeOrient(state.oldPlayer.slopeData.slopeID, player) >= static_cast<int>(SlopeOrientations::OrientUdDown) ? -1 : 1;
        hasSlope = hasSlope && player->velY * mult <= 0;
    }
    
    float deg = RadToDeg(fabsf(slopeAngle(obj, player)));
    float snapHeight = 20 * (deg / 45);
    float min = hasSlope ? -3 : 0;
    
    if (objGetRight(obj) < getLeft(player)) return false;

    switch (gravSlopeOrient(obj, player)) {
        case static_cast<int>(SlopeOrientations::OrientNormalUp):
        case static_cast<int>(SlopeOrientations::OrientNormalDown): {
            float diff = grav(player, expectedSlopeY(obj, player)) - grav(player, player->y);
            return diff >= min && diff <= snapHeight;
        }
        case static_cast<int>(SlopeOrientations::OrientUdUp):
        case static_cast<int>(SlopeOrientations::OrientUdDown): {
            float diffUd = grav(player, player->y) - grav(player, expectedSlopeY(obj, player));  
            return diffUd >= min && diffUd <= snapHeight;
        }
        default:
            return false;
    }
}

}