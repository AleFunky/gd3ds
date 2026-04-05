#pragma once

#include <stdbool.h>
#include "player/player.h"
#include "graphics.h"

#define MAX_LEVEL_HEIGHT 2400.f
#define HITBOX_TRAIL_SIZE 64

typedef struct {
    float x;
    float y;
    float width;
    float height;
    float rotation;
    InternalHitbox internal_hitbox;
} PlayerHitboxTrail;

typedef struct {
    u8 pressedJump:1;
    u8 holdJump:1;
} KeyInput;

typedef struct {
    float camera_x;
    float camera_y;

    float old_camera_x;
    float old_camera_y;

    float camera_intended_y;

    float camera_x_lerp;
    float camera_y_lerp;

    float camera_x_middle;
    float camera_y_middle;

    float intermediate_camera_y;
    
    float camera_wall_timer;
    float camera_wall_initial_y;

    float ground_x;
    float ground_wall_initial_x;

    float background_x;
    float background_wall_initial_x;
    
    Player player;
    Player player2;

    unsigned char current_player;

    Player old_player;

    int mirror_mult;
    float intended_mirror_factor;
    float intended_mirror_speed_factor;
    float mirror_speed_factor;
    float mirror_factor;

    float ground_y;
    float ceiling_y;
    float ground_y_gfx;

    float dual_portal_y;
    unsigned char speed;

    bool noclip;
    int hitbox_display;
    bool dual;
    bool dual_setup;
    bool dead;

    bool profiling;

    float level_progress;

    float timer;
    float death_timer;

    bool custom_level;
    char custom_level_path[256];

    int last_hitbox_trail;
    PlayerHitboxTrail hitbox_trail_players[2][HITBOX_TRAIL_SIZE];

    KeyInput input;
    KeyInput old_input;
} GameState;

#define CEILING_INVUL_TIME 0.15f

extern GameState state;

void set_intended_ceiling();
void set_gamemode(Player *player, int gamemode);
void set_mini(Player *player, bool mini);
void init_variables();
void run_camera();

void handle_death();