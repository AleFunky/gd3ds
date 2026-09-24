#pragma once
#include <citro2d.h>
#include "level_loading.h"
#include "sounds.h"

#include "particles/particles.h"
#include "color_channels.h"

#include "save/saving.h"

#define CAM_SPEED 5.19300155f

#define DT (delta)

#define CAMERA_X_OFFSET (0)
#define CAMERA_X_WALL_OFFSET (2 * 30.F)
#define CAMERA_WALL_ANIM_DURATION 1.f

#define LEVEL_Y_OFFSET 90.f
#define CAM_Y_MTX_OFFSET ((SCREEN_HEIGHT / SCALE - SCREEN_HEIGHT) - LEVEL_Y_OFFSET)


#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define UNUSED __attribute__((unused)) 

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))
#define IN_BOUNDS(index, arr) (index >= 0 && index < ARRAY_LEN(arr))

#define GD_VERSION 1.9
#define LAST_GD_VERSION_ID 11020426 // Last 1.9 id

// When making a release, uncomment this please thanks
// #define IS_RELEASE

typedef struct {
    float x, y;
} Vec2D;

extern float delta;
extern float frame_timer;
extern unsigned int frame_counter;

extern unsigned int level_frame;

extern bool exiting_level;

extern bool song_loaded;

extern bool alt_title_screen;
extern bool is_N3DS;

extern float global_volume;
extern float music_volume;
extern float sound_volume;

extern ParticleSystem touch_drag_particles;
extern ParticleSystem touch_explosion_particles;
extern ParticleSystem glitter_particles_bottom;
extern ParticleSystem slow_speed_particles_bottom;
extern ParticleSystem normal_speed_particles_bottom;
extern ParticleSystem fast_speed_particles_bottom;
extern ParticleSystem faster_speed_particles_bottom;
extern ParticleSystem end_wall_particles;
extern ParticleSystem end_wall_firework;
extern ParticleSystem level_complete_effect_p1;
extern ParticleSystem level_complete_effect_p2;

extern float slow_speed_particles_timer;
extern float normal_speed_particles_timer;
extern float fast_speed_particles_timer;
extern float faster_speed_particles_timer;

#define SCREEN_WIDTH  400
#define SCREEN_BOT_WIDTH  320
#define SCREEN_HEIGHT 240

enum GameState {
    STATE_MENU,
    STATE_GAME,
    STATE_EXIT
};

typedef enum Cheats {
    CHEAT_NOCLIP,
    CHEAT_HITBOX_DISPLAY,
    CHEAT_COUNT
} Cheats;

typedef enum Screens {
    SCREEN_TOP,
    SCREEN_BTM
} Screens;

extern bool cheats_used[CHEAT_COUNT];
extern const char *cheat_names[CHEAT_COUNT];

extern C3D_RenderTarget* top;
extern C3D_RenderTarget* top_right;
extern C3D_RenderTarget* bot;

extern int game_state;
extern bool escape_state;
extern bool playing_menu_loop;
extern char menu_loop_path[32];

extern int level_result;

extern bool cheated;

extern bool game_paused;

extern bool in_level_complete;

extern float accumulator;
extern bool fixed_dt;

void allocate_particles();
void free_particles();
void init_particles(Color p1_color, Color p2_color);
void update_player_effects(float delta);

void apply_volume_settings();
float get_volume_slider();

int output_log(const char *fmt, ...);
u32 jump_key_mask_p1(void);
u32 jump_key_mask_p2(void);
u32 jump_key_mask(void);
void sync_precise_input(bool suppress_held);

bool is_citra();

void load_gdps_info();

extern ServerFile *current_server_file;

extern ExternalLevelFile external_file;
extern ServerFile gd_server_file;
extern ServerFile gdps_file;