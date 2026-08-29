#pragma once

#include <stdbool.h>
#include "player/player.h"
#include "graphics.h"

#define MAX_LEVEL_HEIGHT 2400.f
#define HITBOX_TRAIL_SIZE 64

#define MIRROR_DURATION 0.4f

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

// First flash
#define FLASH_TIME_1 (20.f / STEPS_HZ)
// Rest of timers
#define FLASH_TIME_2 (14.f / STEPS_HZ)

#define RESPAWN_EFFECT_REPEAT 4
#define RESPAWN_EFFECT_DURATION (12.f / STEPS_HZ)

typedef enum {
    DEATH_TITLE_SCREEN_KILL,
    DEATH_SPIKE,
    DEATH_SAW,
    DEATH_BLOCK,
    DEATH_SLOPE,
    DEATH_FELL_OFF_LEVEL,
    DEATH_CEILING,
    DEATH_REASON_COUNT
} DeathReason;

typedef enum {
    FLASH_NONE,
    FLASH_FIRST_LIGHT,
    FLASH_UNLIGHTED,
    FLASH_SECOND_LIGHT
} FlashState;

typedef struct {
    bool flashing;
    FlashState state;
    float timer;
    bool use_lbg;
} BGFlashData;

typedef enum {
    RESPAWN_EFFECT_NONE,
    RESPAWN_EFFECT_SHOW_PLAYER,
    RESPAWN_EFFECT_HIDE_PLAYER
} RespawnEffectState;

typedef struct {
    bool active;
    bool hide_player;
    RespawnEffectState state;
    float timer;
    int remaining;
} RespawnEffectData;

typedef struct {
    int attempts;
    int jumps;
    float time_start;
    float time_end;
    int max_normal;
    int max_practice;
    bool coin1;
    bool coin2;
    bool coin3;
} StateLevelData;

typedef struct {
    bool active;
    float timer;
    float strength;
} ShakeData;

typedef struct {
    float unmod_cam_x;
    float unmod_cam_y;

    float camera_x;
    float camera_y;

    float old_camera_x;
    float old_camera_y;

    float camera_intended_y;

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

    Vec2D attempt_text_pos;

    unsigned char current_player;

    Player old_player;
    Player death_player;

    StateLevelData current_data;

    bool mirroring;
    int mirror_mult;    
    float mirror_timer;

    float original_mirror_factor;
    float intended_mirror_factor;

    float mirror_speed_factor;
    float mirror_factor;

    float ground_y;
    float ceiling_y;
    float ground_y_gfx;

    float dual_portal_y;
    unsigned char speed;

    bool end_wall_anim_playing;
    
    bool noclip;
    int hitbox_display;
    bool dual;
    bool dead;

    bool profiling;
    bool practice_mode;

    float level_progress;

    float timer;
    float death_timer;

    bool hitbox_enabled_when_dead;

    bool online_level;

    bool custom_level;
    char custom_level_path[256];

    BGFlashData flash_data;
    RespawnEffectData respawn_effect_data;
    ShakeData shake_data;

    int last_hitbox_trail;
    PlayerHitboxTrail hitbox_trail_players[2][HITBOX_TRAIL_SIZE];
    
    int p1_trail_pos[2];
    P1Trail p1_trail_data[2][P1_TRAIL_LENGTH];

    KeyInput input;
    KeyInput old_input;

    DeathReason death_reason;
} GameState;

#define CEILING_INVUL_TIME 0.15f

extern GameState state;

void set_intended_ceiling();
void set_gamemode(Player *player, int gamemode);
void set_mini(Player *player, bool mini);
void first_load_init_variables();
void init_variables();
void run_camera();

void handle_death(Player *player, bool pause_song);

void start_bg_flash();
void handle_bg_flash();
void clear_bg_flash();

void start_respawn_effect();
void handle_respawn_effect();
void clear_respawn_effect();

void init_player(Player *player);
void init_state();
void init_trails(int trail);
void init_wave_trails();

void play_level_song(float seek);
void play_practice_song();
void play_menu_song();
void update_attempt_text_pos();

bool is_coin_collected(int obj);

void kill_player(DeathReason reason);

void set_camera_x(float x);
void set_camera_y(float y);

void init_shake();
void start_shake(float duration, float strength);
void handle_shake(float delta);

extern bool disableHitboxesAfterRespawn;