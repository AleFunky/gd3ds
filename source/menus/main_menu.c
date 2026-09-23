#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "math_helpers.h"
#include "main.h"
#include "graphics.h"
#include "state.h"
#include "color_channels.h"
#include "mp3_player.h"

#include "menus/components/ui_list.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_image.h"

#include "menus/palette_kit.h"
#include "menus/main_menu.h"
#include "menus/level_select.h"
#include "menus/settings_hub/settings_hub.h"
#include "menus/settings_hub/settings.h"
#include "menus/statistics.h"
#include "menus/settings_hub/credits.h"
#include "menus/settings_hub/songs.h"
#include "menus/settings_hub/how_to_play.h"
#include "menus/settings_hub/info_card.h"

#include "particles/object_particles.h"
#include "particles/circles.h"
#include "menus/creator_menu/search/server_switcher.h"

#include "save/saving.h"

#define DEATH_WAITING_TIME 0.5f
#define OFFSCREEN_BUFFER 240

static Player title_screen_player;
static bool title_screen_player_hold = false;

static bool pressing = false;
static bool old_pressing = false;

static bool started = false;
static bool holding = false;

static float death_wait_timer = 0;

static int main_menu_color_index = 0;

static float bg_scroll = 0;

bool old_wide;
bool old_stereo;

void action_open_info_card_text(const char *text) {
    // ui_stack_push()
}

void handle_title_screen_player(Player *player) {
    if (state.input.holdJump) {
        if (player->buffering_state == BUFFER_NONE) {
            player->buffering_state = BUFFER_READY;
        }
    } else {
        player->buffering_state = BUFFER_NONE;
    }

    player->on_ground = false;
    player->on_ceiling = false;

    player->velocity_override = false;

    player->gravObj_id = -1;
    player->potentialSlope_id = -1;
    
    player->timeElapsed += STEPS_DT;

    player->upside_down = false;

    player->vel_x = player_speeds[state.speed]; 
    player->x += player->vel_x * STEPS_DT;
    player->y += player->vel_y * STEPS_DT;

    run_player(player);
}

void reset_players() {
    death_wait_timer = 0;

    init_state();
    level_info.wall_x = 9999999999999999999.f;
    level_info.wall_y = 0;
    init_player(&title_screen_player);
    title_screen_player.x = -120;
    title_screen_player.upside_down = false;
    state.speed = random_int(0, SPEED_COUNT - 1);
    title_screen_player.rotation = 0;
    set_gamemode(&title_screen_player, random_int(0, GAMEMODE_COUNT - 1));
    set_mini(&title_screen_player, random_int(0,1));

    title_screen_player.player_icons.cube = random_int(1, ICON_COUNT_PLAYER - 1);
    title_screen_player.player_icons.ship = random_int(1, ICON_COUNT_SHIP - 1);
    title_screen_player.player_icons.ball = random_int(1, ICON_COUNT_PLAYER_BALL - 1);
    title_screen_player.player_icons.ufo  = random_int(1, ICON_COUNT_BIRD - 1);
    title_screen_player.player_icons.wave = random_int(1, ICON_COUNT_DART - 1);
    title_screen_player.player_icons.glow = false;

    title_screen_player_hold = random_int(0,1);
    title_screen_player.y = title_screen_player.height / 2;
    
    Color p1 = get_color_abgr8(colors[random_int(0, NUM_COLORS - 1)]);
    Color p2 = get_color_abgr8(colors[random_int(0, NUM_COLORS - 1)]);

    title_screen_player.player_icons.p1_color = p1;
    title_screen_player.player_icons.p2_color = p2;
    
    init_particles(p1, p2);
    init_trails(0);
    
    trail_p1.color = (random_int(0, 1) ? p1 : p2);
    
    wave_trail_p1.color = (random_int(0, 1) ? p1 : p2);
    wave_trail_p1.blending = true;
    wave_trail_p1.opacity = 1.f;
}

static void handle_input() {
    old_pressing = pressing;
    pressing = false;
    switch (title_screen_player.gamemode) {
        case GAMEMODE_PLAYER:
            // Press A 1/16 of time
            if (!(rand() & 0b1111)) pressing = true;
            break;
        case GAMEMODE_UFO:
            // Press A 1/16 of time
            if (!(rand() & 0b1111)) pressing = true;
            break;
        case GAMEMODE_WAVE:
            // Switch holding A 1/16 of time
            if (!(rand() & 0b1111)) title_screen_player_hold ^= 1;

            if (title_screen_player_hold) {
                pressing = true;
            }
            break;
        case GAMEMODE_SHIP:
            // Switch holding A 1/16 of time
            if (!(rand() & 0b1111)) title_screen_player_hold ^= 1;

            if (title_screen_player_hold) {
                pressing = true;
            }
            break;
    }

    started = !old_pressing && pressing;
    holding = pressing;
}

static void handle_players() {
    state.old_player = title_screen_player;
    handle_title_screen_player(&title_screen_player);

    bool wtrail_opacity_is_zero = (title_screen_player.gamemode == GAMEMODE_WAVE ? wave_trail_p1.opacity <= 0.f : true);
    
    // Wave trail needs to be invisible in order to despawn this player
    if (wtrail_opacity_is_zero && title_screen_player.x >= SCREEN_WIDTH_AREA + OFFSCREEN_BUFFER) {
        MotionTrail_UpdateWaveTrail(&wave_trail_p1, 1.f/60);
        reset_players();
    }

    // Force the player go down by disabling control
    if (title_screen_player.y >= SCREEN_HEIGHT_AREA * 2 - 90) {
        pressing = false;
        title_screen_player_hold = false;
    }
}

static void main_menu_init(UIScreen *s){
    play_menu_song();

    main_menu_color_index = 0;
    u32 color = default_lvl_colors[main_menu_color_index % NUM_MENU_COLORS];
    main_menu_color_index++;

    Color col;
    col.r = GET_R(color);
    col.g = GET_G(color);
    col.b = GET_B(color);

    int chan_bg = get_col_channel_index(CHANNEL_BG);
    int chan_ground = get_col_channel_index(CHANNEL_GROUND);
    int chan_line = get_col_channel_index(CHANNEL_LINE);

    channels[chan_bg].color = col;
    channels[chan_ground].color = col;
    channels[chan_line].color = white;

    get_buffer(CHANNEL_BG)->active = false;
    get_buffer(CHANNEL_GROUND)->active = false;
    get_buffer(CHANNEL_LINE)->active = false;

    allocate_particles();
    init_variables();
    reset_players();

    state.current_player = 0;
    trail = &trail_p1;
    wave_trail = &wave_trail_p1;

    old_wide = settingsState.wideEnabled;
    old_stereo = settingsState.stereoEnabled;
}

static void main_menu_init_top(UIScreen *s){
    UIImage *title = (UIImage *) ui_get_element_by_tag(s, "title");

    if (title && alt_title_screen) {
        ui_image_set_image(title, 3, 1);
    }
}

static void main_menu_update(UIScreen *s, UIInput *input){
    if (input->down & KEY_SELECT) {
        ui_stack_push_game_state(STATE_EXIT);
        stop_mp3();
        return;
    }

    if (settingsState.wideEnabled != old_wide || settingsState.stereoEnabled != old_stereo) {
        gspWaitForVBlank();
        apply_screen_modes();
        gspWaitForVBlank();
        reinitialize_screens();
        old_wide = settingsState.wideEnabled;
        old_stereo = settingsState.stereoEnabled;
    }

    state.old_input = state.input;
    state.input.pressedJump = (started) == true;
    state.input.holdJump = (state.input.pressedJump || holding) == true;
    
    curr_input = state.input;
    curr_old_input = state.old_input;

    for (int i = 0; i < 2; i++) {
        drag_particles[i].emitting = false;
        drag_particles_2[i].stationary = true;
        drag_particles_2[i].emitting = false;
        ship_fire_particles[i].emitting = false;
        secondary_particles[i].emitting = false;
        ship_secondary_particles[i].emitting = false;
        burst_particles[i].emitting = false;
        land_particles[i].emitting = false;
    }
    
    brick_destroy_particles.emitting = false;
    slow_speed_particles_bottom.emitting = false;
    normal_speed_particles_bottom.emitting = false;
    fast_speed_particles_bottom.emitting = false;
    faster_speed_particles_bottom.emitting = false;

    p1_trail = false;

    handle_input();

    if (!state.dead) {
        for (int i = 0; i < 4; i++) {
            handle_players();
        }
    }
    
    glitter_particles.emitting = false;
    glitter_particles_bottom.emitting = false;

    // Fade wave trail
    if (title_screen_player.gamemode == GAMEMODE_WAVE && (state.dead || title_screen_player.x >= SCREEN_WIDTH_AREA)) {
        if (wave_trail->opacity > 0) wave_trail->opacity -= 0.08f;
        
        if (wave_trail->opacity <= 0) {
            wave_trail->opacity = 0;
            wave_trail->nuPoints = 0;
        }
    }

    MotionTrail_Update(&trail_p1, delta);
    MotionTrail_UpdateWaveTrail(&wave_trail_p1, delta);
    update_player_effects(delta);
    update_use_effects(delta, get_use_effect_array_ptr(GFX_TOP));

    float touch_x = input->touchPosition.px/SCALE;
    float touch_y = SCREEN_HEIGHT - input->touchPosition.py/SCALE;

    bool kill = (hidKeysDown() & KEY_TOUCH) && intersect(
        title_screen_player.x, title_screen_player.y, title_screen_player.width, title_screen_player.height, 0,
        touch_x, touch_y, 9, 9, 0
    );

    if (kill && !state.dead) {
        kill_player(DEATH_TITLE_SCREEN_KILL);
        players_destroyed++;
        handle_death(&title_screen_player, false);
        death_wait_timer = DEATH_WAITING_TIME;
    } 

    // Wait to reset the player
    if (death_wait_timer) {
        death_wait_timer -= delta;
        if (death_wait_timer <= 0) {
            reset_players();
            death_wait_timer = 0;
            state.dead = false;
        }
    }

    handle_col_channel(CHANNEL_BG);
    handle_col_channel(CHANNEL_GROUND);

    ColTriggerBuffer *trig = get_buffer(CHANNEL_BG);
    if (!trig->active) {
        upload_color_to_buffer(CHANNEL_BG, default_lvl_colors[main_menu_color_index % NUM_MENU_COLORS], 4.f);
        upload_color_to_buffer(CHANNEL_GROUND, default_lvl_colors[main_menu_color_index % NUM_MENU_COLORS], 4.f);
        main_menu_color_index++;
    }
    
    bg_scroll += 5.19300155f;
}

//bottom screen
static void main_menu_draw(UIScreen *s, UIDrawPhase phase){
    if(phase == UI_DRAW_AFTER) return;

    draw_background(bg_scroll / 8, SCREEN_HEIGHT);
    C2D_ViewScale(SCALE, SCALE);
    state.camera_x = 0;
    state.camera_y = 0;
    draw_player_effects();
    change_blending(true);
    draw_use_effects(get_use_effect_array_ptr(GFX_TOP));

    change_blending(false);
    draw_player(&title_screen_player);
    
    draw_post_player_effects();
    draw_ground(bg_scroll, 0, 0, false, 320);

    C2D_ViewScale(1/SCALE, 1/SCALE);
}

static void main_menu_draw_top(UIScreen *s, UIDrawPhase phase){
    if(phase == UI_DRAW_AFTER) return;

    begin_eye_layer(DEPTH_BACKGROUND);
    draw_background(-40 + (bg_scroll / 8), 0);
    end_eye_layer();

    C2D_ViewScale(SCALE, SCALE);
    state.camera_x = -((SCREEN_WIDTH_AREA - SCREEN_WIDTH_AREA_BOT)/2);
    state.camera_y = SCREEN_HEIGHT_AREA;

    // Same trick as in game, the player floats in front of the screen
    begin_eye_layer(DEPTH_LEVEL);
    draw_player_effects();
    change_blending(true);
    draw_use_effects(get_use_effect_array_ptr(GFX_TOP));

    change_blending(false);
    draw_player(&title_screen_player);
    end_eye_layer();

    C2D_ViewScale(1/SCALE, 1/SCALE);
}

static void main_menu_exit(UIScreen *s) {
    free_particles();
}

const UIScreenDefPair main_menu_def = {
    .name = "main_menu",
    .top = {
        .path = "romfs:/menus/main_menu_top.txt",
        .init = main_menu_init_top,
        .draw = main_menu_draw_top
    },
    .btm = {
        .path = "romfs:/menus/main_menu.txt",
        .init = main_menu_init,
        .update = main_menu_update,
        .draw = main_menu_draw,
        .exit = main_menu_exit
    }
};