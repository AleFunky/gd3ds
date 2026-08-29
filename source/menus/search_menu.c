#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_image.h"
#include "main.h"
#include "graphics.h"

#include "generic_disclaimer.h"
#include "search_menu.h"
#include "server_switcher.h"
#include "search_filters.h"
#include "clear_search_filters.h"
#include "song_filter.h"
#include "menus/components/ui_label.h"
#include "utils/server_utils.h"
#include "menus/components/ui_button.h"

#include "save/saving.h"
#include "save/config.h"

static int new_state = 0;

static bool in_disclaimer = false;
static bool in_server_switcher = false;
static bool in_filters = false;
static bool in_clear_search_filters = false;
static bool exit_flag = false;
bool search_needs_refresh = true;
bool gdps = false;

static UIImage *bg_gradient;
static UIImage *bg_gradient_top;

static void update_difficulty_tint(UIElement *e){
    int tint = (filters.difficultyFilters & (ui_prop_int(&e->custom_properties, "diffValue", 0))) > 0 ? 255 : 127;
    C2D_PlainImageTint(&((UIButton *)e)->image.tint, C2D_Color32(tint, tint, tint, 255), 1.f);
}

void enable_demons(){
    if(gdps) return;

    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "easy")), 259, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "normal")), 261, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "hard")), 257, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "harder")), 263, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "insane")), 265, 0);
}

void disable_demons(){
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "easy")), 252, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "normal")), 253, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "hard")), 254, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "harder")), 255, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(&default_screen, "insane")), 256, 0);
}

void update_difficulty_tints(){
    C2D_PlainImageTint(
        &((UIButton *)ui_get_element_by_tag(&default_screen, "na"))->image.tint, 
        filters.isNA ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    C2D_PlainImageTint(
        &((UIButton *)ui_get_element_by_tag(&default_screen, "auto"))->image.tint, 
        filters.isAuto ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    C2D_PlainImageTint(
        &((UIButton *)ui_get_element_by_tag(&default_screen, "demon"))->image.tint, 
        filters.isDemon ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    ui_run_func_on_tag(&default_screen, "difficulty", update_difficulty_tint);
}

static void action_na(UIElement *e){
    filters.isNA = !filters.isNA;
    if(filters.isNA){
        filters.isAuto = false;
        filters.isDemon = false;
        disable_demons();
        filters.difficultyFilters = 0;
    }
    update_difficulty_tints();
}

static void action_auto(UIElement *e){
    filters.isAuto = !filters.isAuto;
    if(filters.isAuto){
        filters.isNA = false;
        filters.isDemon = false;
        disable_demons();
        filters.difficultyFilters = 0;
    }
    update_difficulty_tints();
}

static void action_demon(UIElement *e){
    filters.isDemon = !filters.isDemon;
    if(filters.isDemon){
        filters.isNA = false;
        filters.isAuto = false;
        filters.difficultyFilters = 0;
        enable_demons();
    } else{
        filters.difficultyFilters = 0;
        disable_demons();
    }
    update_difficulty_tints();
}

static void action_set_difficulty(UIElement *e){
    filters.isNA = false;
    filters.isAuto = false;
    int difficultyVal = ui_prop_int(&e->custom_properties, "diffValue", 0);

    if(filters.isDemon && !gdps) filters.difficultyFilters &= difficultyVal;
    else filters.isDemon = false;

    filters.difficultyFilters ^= difficultyVal;
    update_difficulty_tints();
}

static void action_exit(UIElement *e) {
    exit_flag = true;
    set_fade_status(FADE_STATUS_OUT);
}

void action_open_disclaimer(UIElement* e) {
    in_disclaimer = true;
    disclaimer_init();
}

void action_open_server_switcher(UIElement* e) {
    in_server_switcher = true;
    server_switcher_init();
}

void action_open_filters(UIElement* e) {
    in_filters = true;
    search_filters_init();
}

void action_clear_filters(UIElement* e) {
    in_clear_search_filters = true;
    clear_search_filters_init();
}

void action_set_query(UIElement* e){
    snprintf(filters.searchQuery, sizeof(filters.searchQuery), "%.*s", (int)sizeof(filters.searchQuery) - 1, ((UITextbox *)e)->text);
}

void action_search(UIElement* e) {
    filters.searchType = ui_prop_int(&e->custom_properties, "type", 0);
    filters.currentPage = 0;
    search_needs_refresh = true;
    new_state = STATE_ONLINE;
    set_fade_status(FADE_STATUS_OUT);
}

static UIAction actions[] = {
    {"exit", action_exit },
    {"disclaimer", action_open_disclaimer },
    {"serverswitcher", action_open_server_switcher },
    {"openfilters", action_open_filters },
    {"clearfilters", action_clear_filters },
    {"search", action_search },
    {"searchtext", action_set_query},
    {"difficulty", action_set_difficulty },
    {"na", action_na },
    {"auto", action_auto },
    {"demon", action_demon },
};

void search_menu_loop() {
    exit_flag = false;
    new_state = STATE_SEARCH_MENU;
    
    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_menu.txt");
    bg_gradient = (UIImage *) ui_get_element_by_tag(&default_screen, "gradient");
    ui_load_screen(&default_screen_top, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/search_menu_top.txt");
    bg_gradient_top = (UIImage *) ui_get_element_by_tag(&default_screen_top, "gradient_top");

    UITextbox *searchBox = ((UITextbox *)ui_get_element_by_tag(&default_screen, "searchbox"));
    snprintf(searchBox->text, sizeof(searchBox->text), "%s", filters.searchQuery);

    update_difficulty_tints();
    if(filters.isDemon) enable_demons();

    ui_image_set_tint(bg_gradient, C2D_Color32(50, 110, 255, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(50, 110, 255, 255));

    set_fade_status(FADE_STATUS_IN);

    while (aptMainLoop()) {
        hidScanInput();

        UIInput touch;
        touchPosition touchPos;
        hidTouchRead(&touchPos);
        touch.touchPosition = touchPos;
        touch.did_something = false;
        touch.interacted = false;

        if (!in_disclaimer && !in_server_switcher && !in_clear_search_filters && !in_filters) ui_screen_update(&default_screen, &touch);
        
        // Frees a render target, so keep it out of the frame below
        update_stereo_target();
        
        if (in_filters) { // Bro stop putting this in the rendering do while
            int returned = search_filters_loop();
            if (returned) {
                in_filters = false;
            }
        }

        if (in_server_switcher) {
            int returned = server_switcher_loop();
            if (returned) {
                in_server_switcher = false;
            }
        }

        do {
            update_touch_effect(DT);
            
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            
            // Bottom screen
            C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bot);
            draw_fade();

            ui_screen_draw(&default_screen);
            
            if (in_disclaimer) {
                int returned = disclaimer_loop();
                if (returned) {
                    in_disclaimer = false;
                }
            }

            if (in_clear_search_filters) {
                int returned = clear_search_filters_loop();
                if (returned) {
                    in_clear_search_filters = false;
                }
            }
            
            if (in_filters) search_filters_draw();

            if (in_server_switcher) server_switcher_draw();

            change_blending(true);
            draw_touch_effect();
            change_blending(false);

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
            new_state = STATE_CREATOR_MENU;
        }

        if (new_state != STATE_SEARCH_MENU) {
            cfg_save();
            game_state = new_state;
            break;
        }
    }
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    
    ui_unload_screen(&default_screen);
    ui_unload_screen(&default_screen_top);
}
