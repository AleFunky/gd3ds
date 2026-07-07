#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_online_level_card.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "mp3_player.h"
#include "graphics.h"
#include "state.h"
#include "settings.h"
#include "saved_levels.h"
#include "save/config.h"

static bool exit_flag = false;

static UIElement *bg_gradient;
static UIElement *bg_gradient_top;

static UIElement *list;

typedef struct SavedLevelsEntries {
    char *name;
    char *creator;
    char *song;
    char *length;
    int downloads;
    int likes;
    int stars;
} SavedLevelsEntries;


static const SavedLevelsEntries saved_levels[] = {
    { "Level 1", "Creator", "<#f982ff>Song", "Long", 111, 123, 10 },
    { "hi i am a longer levelname", "superlongcreator", "<#f982ff>Lalalalalala", "Long", 111, 123, 9 },
    { "revolution", "funnygame", "<#f982ff>Something Something Something", "Long", 111, 123, 2 },
};

#define NUM_SAVED_LEVELS_ENTRIES (sizeof(saved_levels) / sizeof(SavedLevelsEntries))

UIElement entries1[NUM_SAVED_LEVELS_ENTRIES];

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static UIAction actions[] = {
    {"exit", action_exit },
};

void saved_levels_loop() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_SceneBegin(bot);
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
    C2D_Fade(0);
    C3D_FrameEnd(0);

    exit_flag = false;

    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/saved_levels.txt");
    bg_gradient = ui_get_element_by_tag(&default_screen, "gradient");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/saved_levels_top.txt");
    bg_gradient_top = ui_get_element_by_tag(&default_screen_top, "gradient_top");

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    list = ui_get_element_by_tag(&default_screen, "list");

    for (int i = 0; i < NUM_SAVED_LEVELS_ENTRIES; i++) {
        char name[256];
        char creator[256];
        char song[256];
        char length[256];
        int downloads = saved_levels[i].downloads;
        int likes = saved_levels[i].likes;
        int stars = saved_levels[i].stars;

        strncpy(name, saved_levels[i].name, sizeof(name) - 1);
        strncpy(creator, saved_levels[i].creator, sizeof(creator) - 1);
        strncpy(song, saved_levels[i].song, sizeof(song) - 1);
        strncpy(length, saved_levels[i].length, sizeof(length) - 1);
        entries1[i] = ui_create_online_level_card(0, 0, i & 1, name, creator, song, length, downloads, likes, stars, NULL);
        ui_list_add(list, &entries1[i]);
    }

    set_fade_status(FADE_STATUS_IN);

    if (!playing_menu_loop) {
        play_mp3("romfs:/songs/menuLoop.mp3", true, 0);
        playing_menu_loop = true;
    }

    while (aptMainLoop()) {
        hidScanInput();

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        ui_screen_update(&default_screen, &touch);
        
        do {
            update_touch_effect(DT);
            
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Bottom screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);
            draw_fade();

            ui_screen_draw(&default_screen);

            change_blending(true);
            draw_touch_effect();
            change_blending(false);

            // Top screen
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(top);
            draw_fade();

            ui_screen_draw(&default_screen_top);
            C2D_ViewReset();
            C3D_FrameEnd(0);
        } while (handle_fading());

        if (exit_flag) {
            game_state = STATE_CREATOR_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
