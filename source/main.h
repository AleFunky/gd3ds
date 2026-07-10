#pragma once
#include <citro2d.h>
#include "level_loading.h"
#include "wav_player.h"

#include "particles/particles.h"
#include "color_channels.h"


#define CAM_SPEED 5.19300155f

#define DT (1.f/60)

#define CAMERA_X_OFFSET (0)
#define CAMERA_X_WALL_OFFSET (2 * 30.F)
#define CAMERA_WALL_ANIM_DURATION 1.f

#define LEVEL_Y_OFFSET 90.f
#define CAM_Y_MTX_OFFSET ((SCREEN_HEIGHT / SCALE - SCREEN_HEIGHT) - LEVEL_Y_OFFSET)


#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define UNUSED __attribute__((unused)) 

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct {
    float x, y;
} Vec2D;

extern float delta;
extern unsigned int frame_counter;

extern unsigned int level_frame;

extern bool song_loaded;

extern bool exiting_level;

extern bool alt_title_screen;

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
    STATE_MAIN_MENU,
    STATE_LEVEL_SELECT,
    STATE_ICON_KIT,
    STATE_GAME,
    STATE_EXTERNAL_LEVELS,
    STATE_SAVED_LEVELS,
    STATE_ONLINE_LEVEL,
    STATE_CREATOR_MENU,
    STATE_SEARCH_MENU,
    STATE_SOGGY,
    STATE_EXIT
};

typedef enum Cheats {
    CHEAT_NOCLIP,
    CHEAT_HITBOX_DISPLAY,
    CHEAT_COUNT
} Cheats;

typedef enum CurrentScreenTarget {
    SCREEN_TOP,
    SCREEN_BOTTOM
} CurrentScreenTarget;

extern bool cheats_used[CHEAT_COUNT];
extern const char *cheat_names[CHEAT_COUNT];

extern C3D_RenderTarget* top;
extern C3D_RenderTarget* bot;

extern int game_state;
extern bool playing_menu_loop;

extern SFX play_sound;
extern SFX quit_sound;
extern SFX explode_sound;
extern SFX end_sound;
extern SFX honk;
extern SFX coin_sound;

extern int level_result;

extern bool cheated;

extern bool game_paused;

extern bool in_level_complete;

void allocate_particles();
void free_particles();
void init_particles(Color p1_color, Color p2_color);
void update_player_effects(float delta);

void apply_volume_settings();
float get_volume_slider();

int output_log(const char *fmt, ...);

bool is_citra();
