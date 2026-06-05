#include <3ds.h>
#include <citro2d.h>

#include <stdlib.h>

#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/palette_kit.h"
#include "fonts/bigFont.h"
#include "fonts/chatFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"

#include "main_menu.h"
#include "level_select.h"
#include "settings.h"
#include "statistics.h"
#include "credits.h"
#include "creator_menu.h"
#include "external_levels.h"
#include "first_boot_disclaimer.h"
#include "info_card.h"
#include "state.h"
#include "particles/object_particles.h"
#include "save/config.h"
#include "particles/circles.h"

#define DEATH_WAITING_TIME 0.5f
#define OFFSCREEN_BUFFER 240

static Player title_screen_player;
static bool title_screen_player_hold = false;

static bool pressing = false;
static bool old_pressing = false;

static bool started = false;
static bool holding = false;

static float death_wait_timer = 0;

static UIScreen screen_top;
static UIScreen screen;

static int main_menu_color_index = 0;

static int new_state = 0;
static bool exit_flag = false;

static bool in_settings = false;
static bool in_statistics = false;
static bool in_credits = false;
static bool in_first_boot_disclaimer = false;
bool in_info_card = false;

static float bg_scroll = 0;

void action_open_level_select(UIElement* e) {
    curr_level_id = 0;
    new_state = STATE_LEVEL_SELECT;
    set_fade_status(FADE_STATUS_OUT);
}
void action_open_creator_menu(UIElement* e) {
    //new_state = STATE_EXTERNAL_LEVELS;
    new_state = STATE_CREATOR_MENU;
    set_fade_status(FADE_STATUS_OUT);
}

void action_open_icon_kit(UIElement* e) {
    new_state = STATE_ICON_KIT;
    set_fade_status(FADE_STATUS_OUT);
}

void action_open_settings(UIElement* e) {
    in_settings = true;
    settings_init();
}

void action_open_statistics(UIElement* e) {
    in_statistics = true;
    statistics_init();
}

void action_open_credits(UIElement* e) {
    in_credits = true;
    credits_init();
}

void action_open_info_card(int id, UIElement* e) {
    info_card_init();
    switch (id) {
        case 1:
            // wide mode info
            set_info_content("Doubles the top screen's horizontal<p>resolution.");
            break;
        case 2:
            // global tap effect info
            set_info_content("Plays the tap effect across all menus.");
            break;
        case 3:
            // more jump buttons info
            set_info_content("Swaps your jump input to Y.");
            break;
        case 4:
            // hitboxes info
            set_info_content("Shows object hitboxes while in a level<p>WARNING: AFFECTS PERFORMANCE!");
            break;
        case 5:
            // debug info
            set_info_content("Enables debug key shortcuts.<p>(B + L, B + R, X)");
            break;
        case 6:
            // accurate percentage info
            set_info_content("Shows level progress with 2 decimals.");
            break;
        case 7:
            // ULTRA accurate percentage info
            set_info_content("But mom! I want more decimals!!!!<p>(use at your own risk)");
            break;
        case 8:
            // Switch trail color
            set_info_content("Makes the player trail use P1<p>instead of P2.");
            break;
        case 9:
            // Switch wave trail color
            set_info_content("Makes the wave trail use P1<p>instead of P2.");
            break;
        case 10:
            // quick retry info
            set_info_content("Restarts in 0,5 seconds instead of 1.");
            break;
        case 11:
            // solid trail info
            set_info_content("Disables blending for the wave trail.");
            break;
        case 12:
            // no wave trail behind info
            set_info_content("Disables player trail for the wave.");
            break;
        case 13:
            // do not info
            set_info_content("Doesn't do anything...<p>Well, nothing useful.");
            break;
    }
    in_info_card = true;
}

static UIAction actions[] = {
    { "level_select", action_open_level_select },
    { "creator_menu", action_open_creator_menu },
    { "settings", action_open_settings },
    { "statistics", action_open_statistics },
    { "icon_kit", action_open_icon_kit },
    { "credits", action_open_credits },
};

static UIAction actions_top[] = {

};

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
        case GAMEMODE_BIRD:
            // Press A 1/16 of time
            if (!(rand() & 0b1111)) pressing = true;
            break;
        case GAMEMODE_DART:
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

    bool wtrail_opacity_is_zero = (title_screen_player.gamemode == GAMEMODE_DART ? wave_trail_p1.opacity <= 0.f : true);
    
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

void main_menu_loop() {
    exit_flag = false;
    new_state = 0;
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/main_menu.txt");
    ui_load_screen(&screen_top, actions_top, sizeof(actions_top) / sizeof(actions_top[0]), "romfs:/menus/main_menu_top.txt");
    
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

    UIElement *title = ui_get_element_by_tag(&screen_top, "title");

    if (title && alt_title_screen) {
        ui_image_set_image(title, 3, 1);
    }

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
        playing_menu_loop = true;
    }

    get_buffer(CHANNEL_BG)->active = false;
    get_buffer(CHANNEL_GROUND)->active = false;
    get_buffer(CHANNEL_LINE)->active = false;

    allocate_particles();
    init_variables();
    reset_players();

    state.current_player = 0;
    trail = &trail_p1;
    wave_trail = &wave_trail_p1;

    bool old_wide = wideEnabled;
    
    if (initialDisclaimerAccepted == false) {
        in_first_boot_disclaimer = true;
        first_boot_disclaimer_init();
    }

    while (aptMainLoop()) {
        float delta = 1/60.f;
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_SELECT) {
            game_state = STATE_EXIT;
            stop_mp3();
            break; // break in order to return to hbmenu
        }

        if (kDown & KEY_START) {
            action_open_level_select(NULL);
        }


        state.old_input = state.input;
        state.input.pressedJump = (started) == true;
        state.input.holdJump = (state.input.pressedJump || holding) == true;

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
        if (title_screen_player.gamemode == GAMEMODE_DART && (state.dead || title_screen_player.x >= SCREEN_WIDTH_AREA)) {
            if (wave_trail->opacity > 0) wave_trail->opacity -= 0.08f;
            
            if (wave_trail->opacity <= 0) {
                wave_trail->opacity = 0;
                wave_trail->nuPoints = 0;
            }
        }
    
        MotionTrail_Update(&trail_p1, delta);
        MotionTrail_UpdateWaveTrail(&wave_trail_p1, delta);
        update_player_effects(delta);
        update_use_effects(delta, GFX_TOP);

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        float touch_x = touchPos.px/SCALE;
        float touch_y = SCREEN_HEIGHT - touchPos.py/SCALE;

        bool kill = (hidKeysDown() & KEY_TOUCH) && intersect(
            title_screen_player.x, title_screen_player.y, title_screen_player.width, title_screen_player.height, 0,
            touch_x, touch_y, 9, 9, 0
        );

        // Ded
        if (kill && !state.dead) {
            state.dead = true;
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

        if (wideEnabled != old_wide) {        
            gspWaitForVBlank();
            set_wide(wideEnabled);
            gspWaitForVBlank();
            reinitialize_screens();
            old_wide = wideEnabled;
        }

        if (!in_settings && !in_credits && !in_statistics && !in_first_boot_disclaimer && !in_info_card) ui_screen_update(&screen, &touch);
        ui_screen_update(&screen_top, &touch);
        do {
            update_touch_effect(DT);
            
            bg_scroll += 5.19300155f;
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Top screen
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(top);
            draw_fade();

            draw_background(-40 + (bg_scroll / 8), 0);
            C2D_ViewScale(SCALE, SCALE);
            state.camera_x = -((SCREEN_WIDTH_AREA - SCREEN_WIDTH_AREA_BOT)/2);
            state.camera_y = SCREEN_HEIGHT_AREA;

            draw_player_effects();
            change_blending(true);
            draw_use_effects(GFX_TOP);

            change_blending(false);
            draw_player(&title_screen_player);

            C2D_ViewScale(1/SCALE, 1/SCALE);
            ui_screen_draw(&screen_top);

            // Bottom Screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);

            draw_background(bg_scroll / 8, SCREEN_HEIGHT);
            C2D_ViewScale(SCALE, SCALE);
            state.camera_x = 0;
            state.camera_y = 0;
            draw_player_effects();
            change_blending(true);
            draw_use_effects(GFX_TOP);

            change_blending(false);
            draw_player(&title_screen_player);
            
            draw_post_player_effects();
            draw_ground(bg_scroll, 0, 0, false, 320);

            C2D_ViewScale(1/SCALE, 1/SCALE);

            ui_screen_draw(&screen);
            if (in_settings) {
                int returned = settings_loop();
                if (returned) {
                    in_settings = false;
                }
            }

            if (in_statistics) {
                int returned = statistics_loop();
                if (returned) {
                    in_statistics = false;
                }
            }

            if (in_credits) {
                int returned = credits_loop();
                if (returned) {
                    in_credits = false;
                }
            }

            if (in_first_boot_disclaimer) {
                int returned = first_boot_disclaimer_loop();
                if (returned) {
                    in_first_boot_disclaimer = false;
                }
            }

            if (in_info_card) {
                int returned = info_card_loop();
                if (returned) {
                    in_info_card = false;
                }
            }

            change_blending(true);
            draw_touch_effect();
            change_blending(false);

            C2D_ViewReset();

            C3D_FrameEnd(0);
        } while (handle_fading());

        if (new_state) {
            game_state = new_state;
            free_particles();
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
}
