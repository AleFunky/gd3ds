#pragma once

#include <stdbool.h>
#include "level_loading.h"
#include "icons.h"
#include "particles/particles.h"

#include "trail.h"

extern int frame_skipped;

#define STEPS_HZ 240
#define STEPS_DT ((1.f + frame_skipped) / STEPS_HZ) // 1/240 seconds per physics step
#define STEPS_DT_UNMOD (1.f / STEPS_HZ) // 1/240 seconds per physics step

#define P1_TRAIL_LENGTH 10
#define P1_TRAIL_DURATION 0.45f
#define P1_TRAIL_END_SCALE 0.8f

#define BURST_PARTICLES_DURATION 0.15f

#define DRAG_PARTICLES_FLOOR_DURATION 0.1f

#define BALL_SLOW_ROTATION 0.7f

typedef struct {
    int slope_id;
    float elapsed;
    bool snapDown;
} SlopeData;

typedef struct {
    int object_id;
    int player_frame;
    float player_snap_diff;
    int snapped_obj;
} SnapData;

typedef struct {
    float width;
    float height;
} InternalHitbox;

typedef struct {
    int gamemode;
    float x;
    float y;
    float rot;

    float scale;
    float delta_scale;

    float opacity;

    float life;

    bool upside_down;
    bool active;
} P1Trail;

typedef struct {
    float x;
    float y;
    
    float vel_x;
    float vel_y;

    float delta_y;
    
    float gravity;

    float rotation;
    float lerp_rotation;
    
    float width;
    float height;

    InternalHitbox internal_hitbox;

    int gamemode;
    
    int rotation_direction;

    bool on_ground;
    bool on_ceiling;
    bool mini;
    bool upside_down;
    bool touching_slope;
    bool inverse_rotation;
    bool snap_rotation;
    
    int potentialSlope_id;

    bool left_ground;

    float ball_rotation_speed;

    float cutscene_timer;

    int buffering_state;

    float time_since_ground;
    
    float ufo_last_y;

    float ceiling_inv_time;

    float timeElapsed;

    int gravObj_id;

    float burst_particle_timer;

    float cutscene_initial_player_x;
    float cutscene_initial_player_y;

    int slope_slide_coyote_time;

    int frame;

    SlopeData coyote_slope;
    SlopeData slope_data;

    SnapData snap_data;

    bool velocity_override;

    float coyote_frames;
    
    int p1_trail_pos;
    P1Trail p1_trail_data[P1_TRAIL_LENGTH];
} Player;

enum BufferingState {
    BUFFER_NONE,
    BUFFER_READY,
    BUFFER_END
};

enum PlayerSpeeds {
    SPEED_SLOW,
    SPEED_NORMAL,
    SPEED_FAST,
    SPEED_FASTER,
    SPEED_COUNT
};

extern float collision_time;
extern float player_time;
extern float handle_player_time;

extern MotionTrail *trail;
extern MotionTrail trail_p1;
extern MotionTrail trail_p2;

extern MotionTrail *wave_trail;
extern MotionTrail wave_trail_p1;
extern MotionTrail wave_trail_p2;

extern ParticleSystem drag_particles[2];
extern ParticleSystem drag_particles_2[2];
extern ParticleSystem ship_fire_particles[2];
extern ParticleSystem ship_secondary_particles[2];
extern ParticleSystem secondary_particles[2];
extern ParticleSystem burst_particles[2];
extern ParticleSystem land_particles[2];
extern ParticleSystem explosion_particles[2];
extern ParticleSystem glitter_particles;
extern ParticleSystem slow_speed_particles;
extern ParticleSystem normal_speed_particles;
extern ParticleSystem fast_speed_particles;
extern ParticleSystem faster_speed_particles;
extern ParticleSystem coin_pickup_particles;

extern const float player_speeds[SPEED_COUNT];

inline float getTop(Player *player)  { return player->y + player->height / 2; }
inline float getBottom(Player *player)  { return player->y - player->height / 2; }

inline float getGroundTop(Player *player)  { return player->y + (player->height / 2) + ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0); }
inline float getGroundBottom(Player *player)  { return player->y - (player->height / 2) - ((player->gamemode == GAMEMODE_DART) ? (player->mini ? 3 : 5) : 0); }

inline float getRight(Player *player)  { return player->x + player->width / 2; }
inline float getLeft(Player *player)  { return player->x - player->width / 2; }

inline float getInternalTop(Player *player)  { return player->y + player->internal_hitbox.height / 2; }
inline float getInternalBottom(Player *player)  { return player->y - player->internal_hitbox.height / 2; }
inline float getInternalRight(Player *player)  { return player->x + player->internal_hitbox.width / 2; }
inline float getInternalLeft(Player *player)  { return player->x - player->internal_hitbox.width / 2; }

inline float gravBottom(Player *player) { return player->upside_down ? -getTop(player) : getBottom(player); }
inline float gravTop(Player *player) { return player->upside_down ? -getBottom(player) : getTop(player); }

inline float gravInternalBottom(Player *player) { return player->upside_down ? -getInternalTop(player) : getInternalBottom(player); }
inline float gravInternalTop(Player *player) { return player->upside_down ? -getInternalBottom(player) : getInternalTop(player); }

inline float grav(Player *player, float val) { return player->upside_down ? -val : val; }

inline float obj_getTop(int object)  { 
    return objects.y[object] + objects.height[object] / 2; 
}
inline float obj_getBottom(int object)  { 
    return objects.y[object] - objects.height[object] / 2; 
}
inline float obj_getRight(int object)  {  
    return objects.x[object] + objects.width[object] / 2; 
}
inline float obj_getLeft(int object)  { 
    return objects.x[object] - objects.width[object] / 2; 
}
inline float obj_gravBottom(Player *player, int object) { return player->upside_down ? -obj_getTop(object) : obj_getBottom(object); }
inline float obj_gravTop(Player *player, int object) { return player->upside_down ? -obj_getBottom(object) : obj_getTop(object); }

void handle_player(Player *player);
void draw_player(Player *player);
void run_player(Player *player);

void draw_hitbox(int obj);
void draw_player_hitbox(Player *player);
void draw_hitbox_trail(int player);
void add_new_hitbox(Player *player);

void update_p1_trail(Player *player);

void update_rotation_direction(Player *player);