#pragma once
#include <citro2d.h>
#include "level_loading.h"
#include "wav_player.h"

#include "particles/particles.h"


#define CAM_SPEED 5.19300155f

#define DT (1.f/60)

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

extern float delta;
extern unsigned int frame_counter;

extern unsigned int level_frame;

extern bool song_loaded;

extern bool exiting_level;

extern bool alt_title_screen;

extern ParticleSystem touch_drag_particles;
extern ParticleSystem touch_explosion_particles;

extern float slow_speed_particles_timer;
extern float normal_speed_particles_timer;
extern float fast_speed_particles_timer;
extern float faster_speed_particles_timer;

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

enum GameState {
    STATE_MAIN_MENU,
    STATE_LEVEL_SELECT,
    STATE_ICON_KIT,
    STATE_GAME,
    STATE_EXTERNAL_LEVELS,
    STATE_CREATOR_MENU,
    STATE_EXIT
};

extern C3D_RenderTarget* top;
extern C3D_RenderTarget* bot;

extern int game_state;
extern bool playing_menu_loop;

extern SFX play_sound;
extern SFX quit_sound;
extern SFX explode_sound;

int output_log(const char *fmt, ...);

bool is_citra();
