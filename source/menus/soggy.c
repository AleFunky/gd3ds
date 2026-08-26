#include <3ds.h>
#include <citro2d.h>
#include "3ds/thread.h"
#include "3ds/types.h"
#include "fonts/bigFont.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_list.h"
#include "main.h"
#include "mp3_player.h"
#include "graphics.h"

#include "save/config.h"
#include "text.h"
#include "utils/json_config.h"
#include "utils/network.h"

static bool exit_flag = false;

bool gotSogged = false;

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

static void action_boop(UIElement *e) {
    play_sfx(&honk, 1);
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"boop", action_boop},
};

void soggy_menu_loop() {
    exit_flag = false;
    gotSogged = true;
    cfg_save(); // You got sogged

    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/soggy.txt");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/soggy_top.txt");

    set_fade_status(FADE_STATUS_IN);

    stop_mp3();
    play_mp3("romfs:/songs/SogLoop.mp3", true, 0);

    // START TEST

    DownloadTask task = {
        .path = USER_SONGS_DIR,
        .url = "https%3A%2F%2Faudio.ngfiles.com%2F803000%2F803223_Xtrullor---Arcana.mp3%3Ff1524940372",
        .song_id = "803223"
    };

    Thread thread = create_download_song_thread(&task);

    // END TEST

    while (aptMainLoop()) {
        hidScanInput();

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        ui_screen_update(&default_screen, &touch);

        // START TEST
        
        if (task.running) {
            //output_log("Progress: %.2f\n", task.progress);
        }

        if (task.finished) {
            //output_log("Download finished with code %d\n", task.result);
            task.finished = false;
        }
        
        // END TEST

        // Frees a render target, so keep it out of the frame below
        update_stereo_target();

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

            // START TEST
            
            draw_text(&bigFont_fontCharset, &bigFont_sheet, 0, 200, 0.5f, 0.5f, 0.f, true, "Progress %.2f Finished %d", task.progress, !task.running);

            // END TEST

            // Top screen, drawn once per eye when 3D is on
            for (int eye = 0; begin_top_eye(eye); eye++) {
                draw_fade();

                begin_eye_layer(DEPTH_UI);
                ui_screen_draw(&default_screen_top);
                end_eye_layer();
            }
            C2D_ViewReset();
            C3D_FrameEnd(0);
        } while (handle_fading());

        if (exit_flag) {
            stop_mp3();

            // START TEST

            // Cancel task
            if (task.running) {
                task.cancelled = true;
                threadJoin(thread, U64_MAX);
            }

            // END TEST

            game_state = STATE_CREATOR_MENU;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
