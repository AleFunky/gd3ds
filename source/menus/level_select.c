#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include "level_loading.h"
#include "menus/core/common_setters.h"

#include "menus/core/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_window_button.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_progress_bar.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "level_select.h"

#include "level/main_levels.h"
#include "save/saving.h"

#include "menus/gameplay.h"

#include "state.h"

const char *error_strings[LOAD_ERROR_COUNT - 1] = {
    "Invalid gmd.",
    "Invalid level data.",
    "Level string missing sections.",
    "Out of memory.",
    "Couldn't parse objects.",
    "Invalid Base64."
};

static bool start_level = false;

int curr_level_id = 0;

s8 scroll_dir = 0;

float anim_duration = 1.f;
float anim_time = 0;

static bool dragging;
static int dragStartX;
static int lastTouchX;
static int dragDistance;
static int dragDir;
//Prevents cards from overlapping 
//hopefully
static bool cardCorrection;

static UIImage *bg_gradient = NULL;
static UIImage *bg_gradient_top = NULL;
static UIWindowButton *level_card_window = NULL;
static UILabel *level_card_title_top = NULL;

static UILabel *level_card_title = NULL;
static UILabel *level_card_stars = NULL;
static UIImage *level_card_face = NULL;

static UIWindowButton *level_card_2_window = NULL;

static UILabel *level_card_2_title = NULL;
static UILabel *level_card_2_stars = NULL;

static UIProgressBar *level_card_normal_progress = NULL;
static UILabel *level_card_normal_progress_val = NULL;
static UIProgressBar *level_card_2_normal_progress = NULL;
static UILabel *level_card_2_normal_progress_val = NULL;

static UIProgressBar *level_card_practice_progress = NULL;
static UILabel *level_card_practice_progress_val = NULL;
static UIProgressBar *level_card_2_practice_progress = NULL;
static UILabel *level_card_2_practice_progress_val = NULL;

static UIImage *level_card_coin_1 = NULL;
static UIImage *level_card_coin_2 = NULL;
static UIImage *level_card_coin_3 = NULL;
static UIImage *level_card_2_coin_1 = NULL;
static UIImage *level_card_2_coin_2 = NULL;
static UIImage *level_card_2_coin_3 = NULL;

#define ANIM_DURATION 0.8f
#define COLOR_FADE_DURATION 0.1f

#define C2D_Color32Const(r, g, b, a) (r | (g << (u32)8) | (b << (u32)16) | (a << (u32)24))

const u32 default_lvl_colors[] = {
    C2D_Color32Const(0, 0, 232, 255),
    C2D_Color32Const(227, 0, 229, 255), 
    C2D_Color32Const(233, 0, 115, 255),
    C2D_Color32Const(233, 0, 0, 255),
    C2D_Color32Const(231, 112, 0, 255),
    C2D_Color32Const(233, 232, 0, 255),
    C2D_Color32Const(0, 231, 0, 255),
    C2D_Color32Const(0, 227, 228, 255),
    C2D_Color32Const(0, 112, 229, 255),
};

const size_t NUM_MENU_COLORS = ARRAY_LEN(default_lvl_colors);

UIScreen *top_screen = NULL;
UIScreen *btm_screen = NULL;

#define DOTS_SCALE 0.75f

#define DOTS_X (SCREEN_WIDTH / 2)
#define DOTS_Y 232
#define DOTS_WIDTH 8
#define DOTS_MARGIN 8

static void draw_dots(int level_id) {
    C2D_Sprite spr = { 0 };
    C2D_SpriteFromSheet(&spr, ui_sheet, 421);
    C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    C2D_SpriteSetScale(&spr, DOTS_SCALE, DOTS_SCALE);
    
    C2D_ImageTint tint = {0};

    C2D_PlainImageTint(&tint, C2D_Color32(125, 125, 125, 255), 1.f);

    int left_side = DOTS_X - ((DOTS_WIDTH * MAIN_LEVELS_NUM) + (DOTS_MARGIN * (MAIN_LEVELS_NUM - 1))) * DOTS_SCALE / 2;

    for (int i = 0; i < MAIN_LEVELS_NUM; i++) {
        C2D_SpriteSetPos(&spr, left_side + i * (DOTS_WIDTH + DOTS_MARGIN) * DOTS_SCALE, DOTS_Y);
        
        // Do not tint if current level
        if (i == level_id) {
            C2D_DrawSprite(&spr);
        } else {
            C2D_DrawSpriteTinted(&spr, &tint);
        }
    }
}

void update_level_name(int level, int card);
void update_level_stars(int level, int card);

void disable_card_2(UIElement *e) {
    e->enabled = false;
}

void enable_card_2(UIElement *e) {
    e->enabled = true;
}

void level_card_move_right(UIElement *e) {
    e->x += 320;
}

void level_card_move_left(UIElement *e) {
    e->x -= 320;
}

void update_level_progress(int level, int card) {
    if (level < 0) level = MAIN_LEVELS_NUM-1;
    if (level >= MAIN_LEVELS_NUM) level = 0;

    LevelData *data = &main_level_data[level]; 

    char normal[256];
    snprintf(normal, sizeof(normal), "<#ff00ff>Normal</>: %d%%", data->normal_progress);
    
    char practice[256];
    snprintf(practice, sizeof(practice), "<#ffa54b>Practice</>: %d%%", data->practice_progress);

    UIProgressBar *normal_prog = (card) ? level_card_2_normal_progress : level_card_normal_progress;
    UIProgressBar *practice_prog = (card) ? level_card_2_practice_progress : level_card_practice_progress;
    UILabel *normal_progval = (card) ? level_card_2_normal_progress_val : level_card_normal_progress_val;
    UILabel *practice_progval = (card) ? level_card_2_practice_progress_val : level_card_practice_progress_val;

    UIImage *coin_1 = (card) ? level_card_2_coin_1 : level_card_coin_1;
    UIImage *coin_2 = (card) ? level_card_2_coin_2 : level_card_coin_2;
    UIImage *coin_3 = (card) ? level_card_2_coin_3 : level_card_coin_3;

    normal_prog->value = data->normal_progress;
    practice_prog->value = data->practice_progress;

    snprintf(normal, sizeof(normal), "%d%%", data->normal_progress);
    snprintf(practice, sizeof(practice), "%d%%", data->practice_progress);

    ui_label_set_text(normal_progval, normal);
    ui_label_set_text(practice_progval, practice);
    
    ui_image_set_image(coin_1, (data->coin1 ? MENU_COIN_FILLED_ID : MENU_COIN_UNFILLED_ID), 0);
    ui_image_set_image(coin_2, (data->coin2 ? MENU_COIN_FILLED_ID : MENU_COIN_UNFILLED_ID), 0);
    ui_image_set_image(coin_3, (data->coin3 ? MENU_COIN_FILLED_ID : MENU_COIN_UNFILLED_ID), 0);
}

void update_level_name(int level, int card) {
    if (level < 0) level = MAIN_LEVELS_NUM-1;
    if (level >= MAIN_LEVELS_NUM) level = 0;

    UILabel *e = (card) ? level_card_2_title : level_card_title;
    level_card_title_top = (UILabel *) ui_get_element_by_tag(top_screen, "levelname");

    ui_element_set_scale((UIElement *) e, 1.f);

    ui_label_set_text(e, main_levels[level].level_name);
}

void update_level_stars(int level, int card) {
    if (level < 0) level = MAIN_LEVELS_NUM-1;
    if (level >= MAIN_LEVELS_NUM) level = 0;

    UILabel *e = (card) ? level_card_2_stars : level_card_stars;
    char stars[10] = { 0 };
    snprintf(stars, 9, "%d", main_levels[level].stars);
    ui_label_set_text(e, stars);
}

void update_level_face(int level) {
    if (level < 0) level = MAIN_LEVELS_NUM-1;
    if (level >= MAIN_LEVELS_NUM) level = 0;

    ui_image_set_image(level_card_face, 239 + main_levels[level].difficulty, 0);
}
void update_level_top(int level){
    LevelData *data = &main_level_data[level]; 

    char attempts[256];
    snprintf(attempts, sizeof(attempts), "<#40e348>Total Attempts</>: %d", data->attempts);
    
    char jumps[256];
    snprintf(jumps, sizeof(jumps), "<#60abef>Total Jumps</>: %d", data->jumps);

    char normal[256];
    snprintf(normal, sizeof(normal), "<#ff00ff>Normal</>: %d%%", data->normal_progress);
    
    char practice[256];
    snprintf(practice, sizeof(practice), "<#ffa54b>Practice</>: %d%%", data->practice_progress);

    ui_label_set_text(level_card_title_top, main_levels[level].level_name);
    ui_label_set_text((UILabel *) ui_get_element_by_tag(top_screen, "totalattempts"), attempts);
    ui_label_set_text((UILabel *) ui_get_element_by_tag(top_screen, "totaljumps"), jumps);
    ui_label_set_text((UILabel *) ui_get_element_by_tag(top_screen, "normalprogress"), normal);
    ui_label_set_text((UILabel *) ui_get_element_by_tag(top_screen, "practiceprogress"), practice);
}

void recenter(){
    update_level_name(curr_level_id, 0);
    update_level_stars(curr_level_id, 0);
    update_level_progress(curr_level_id, 0);
}

void handle_card_movement() {
    if (!dragging) {
        if (anim_time > anim_duration) {
            update_level_name(curr_level_id, 0);
            update_level_stars(curr_level_id, 0);
            update_level_progress(curr_level_id, 0);

            ui_run_func_on_tag(btm_screen, "level_card_2", disable_card_2);
            ui_set_pos_on_tag(btm_screen, 160, LEVEL_CARD_Y_POS, "level_card");
            ui_set_pos_on_tag(btm_screen, 160, LEVEL_CARD_Y_POS, "level_card_2");
            scroll_dir = 0;
            dragDistance = 0;

            return;
        }

        float start = (scroll_dir == 0) ? dragDistance : dragDistance * scroll_dir;
        float end = (scroll_dir == 0) ? 0 : 320;

        float fade_value = easeValue(ELASTIC_OUT, start, end, anim_time, anim_duration, 0.6f);

        float value = (scroll_dir == 0) ? 160 + fade_value : 160 + fade_value * scroll_dir;
        anim_time += 0.016666f;

        ui_set_pos_on_tag(btm_screen, value, LEVEL_CARD_Y_POS, "level_card");
        ui_set_pos_on_tag(btm_screen, value - 320 * scroll_dir, LEVEL_CARD_Y_POS, "level_card_2");
    }
}

void lerp_level_colors(u32 color1, u32 color2){
    float frac = abs(dragDistance) / 320.f;
    frac = clampf(frac, 0.f, 1.f);
    u32 lerpCol = color_lerp_u32(color1, color2, frac);
    upload_color_to_buffer(0, lerpCol, 0);
}

void peek_right(){
    ui_run_func_on_tag(btm_screen, "level_card_2", enable_card_2);

    int card2id = curr_level_id + 1;
    if (card2id >= MAIN_LEVELS_NUM) card2id = 0;

    update_level_name(card2id, 1);
    update_level_stars(card2id, 1);
    update_level_progress(card2id, 1);

    u32 col1 = default_lvl_colors[curr_level_id % NUM_MENU_COLORS];
    u32 col2 = default_lvl_colors[card2id % NUM_MENU_COLORS];
    lerp_level_colors(col1, col2);
}

void peek_left(){
    ui_run_func_on_tag(btm_screen, "level_card_2", enable_card_2);

    int card2id = curr_level_id - 1;
    if (card2id < 0) card2id = MAIN_LEVELS_NUM-1;

    update_level_name(card2id, 1);
    update_level_stars(card2id, 1);
    update_level_progress(card2id, 1);

    u32 col1 = default_lvl_colors[curr_level_id % NUM_MENU_COLORS];
    u32 col2 = default_lvl_colors[card2id % NUM_MENU_COLORS];
    lerp_level_colors(col1, col2);
}

void tint_ground(UIElement *e) {
    ColorChannel channel = channels[0];
    ui_image_set_tint((UIImage *) e, C2D_Color32(channel.color.r, channel.color.g, channel.color.b, 255));
}

void action_open_level(UIElement* e, const UIPropertyList *args) { 
    stop_mp3();
    playing_menu_loop = false;
    ui_stack_push_game_state(STATE_GAME);
}

void action_move_right(UIElement* e, const UIPropertyList *args) { 
    curr_level_id++;
    scroll_dir = -1;
    anim_time = 0;
    anim_duration = 1.f;
    
    if (curr_level_id >= MAIN_LEVELS_NUM) curr_level_id = 0;
    
    ui_set_pos_on_tag(btm_screen, 160, LEVEL_CARD_Y_POS, "level_card");
    ui_run_func_on_tag(btm_screen, "level_card_2", enable_card_2);
    ui_run_func_on_tag(btm_screen, "level_card_2", level_card_move_right);
    
    upload_color_to_buffer(0, default_lvl_colors[curr_level_id % NUM_MENU_COLORS], COLOR_FADE_DURATION);

    update_level_name(curr_level_id - 1, 0);
    update_level_stars(curr_level_id - 1, 0);
    update_level_progress(curr_level_id - 1, 0);
    
    update_level_face(curr_level_id);
    update_level_top(curr_level_id);

    update_level_name(curr_level_id, 1);
    update_level_stars(curr_level_id, 1);
    update_level_progress(curr_level_id, 1);
};

void action_move_left(UIElement* e, const UIPropertyList *args) { 
    curr_level_id--;
    scroll_dir = 1;
    anim_time = 0;
    anim_duration = 1.f;

    if (curr_level_id < 0) curr_level_id = MAIN_LEVELS_NUM-1;

    ui_set_pos_on_tag(btm_screen, 160, LEVEL_CARD_Y_POS, "level_card");
    ui_run_func_on_tag(btm_screen, "level_card_2", enable_card_2);
    ui_run_func_on_tag(btm_screen, "level_card_2", level_card_move_left);

    upload_color_to_buffer(0, default_lvl_colors[curr_level_id % NUM_MENU_COLORS], COLOR_FADE_DURATION);

    update_level_name(curr_level_id + 1, 0);
    update_level_stars(curr_level_id + 1, 0);
    update_level_progress(curr_level_id + 1, 0);
    
    update_level_face(curr_level_id);
    update_level_top(curr_level_id);

    update_level_name(curr_level_id, 1);
    update_level_stars(curr_level_id, 1);
    update_level_progress(curr_level_id, 1);
}

static UIActionDef actions[] = {
    {"open_level", action_open_level},
    {"move_right", action_move_right},
    {"move_left", action_move_left}
};

int mode = 0;

void level_select_init(UIScreen *s){
    btm_screen = s;
    
    curr_level_id = 0;
    start_level = false;
    state.custom_level = false;
    dragDistance = 0;

    // Set window color
    level_card_window = (UIWindowButton *) ui_get_element_by_tag(s, "card_window");
    ui_window_button_set_tint(level_card_window, C2D_Color32(0, 0, 0, 127));

    level_card_2_window = (UIWindowButton *) ui_get_element_by_tag(s, "card_window_2");
    ui_window_button_set_tint(level_card_2_window, C2D_Color32(0, 0, 0, 127));

    // Get level card components
    level_card_title = (UILabel *) ui_get_element_by_tag(s, "level_title");
    level_card_stars = (UILabel *) ui_get_element_by_tag(s, "level_stars");

    level_card_2_title = (UILabel *) ui_get_element_by_tag(s, "level_title_2");
    level_card_2_stars = (UILabel *) ui_get_element_by_tag(s, "level_stars_2");

    level_card_normal_progress = (UIProgressBar *) ui_get_element_by_tag(s, "normalprogress");
    level_card_normal_progress_val = (UILabel *) ui_get_element_by_tag(s, "normalprogressvalue");
    level_card_2_normal_progress = (UIProgressBar *) ui_get_element_by_tag(s, "normalprogress_2");
    level_card_2_normal_progress_val = (UILabel *) ui_get_element_by_tag(s, "normalprogressvalue_2");
    
    level_card_practice_progress = (UIProgressBar *) ui_get_element_by_tag(s, "practiceprogress");
    level_card_practice_progress_val = (UILabel *) ui_get_element_by_tag(s, "practiceprogressvalue");
    level_card_2_practice_progress = (UIProgressBar *) ui_get_element_by_tag(s, "practiceprogress_2");
    level_card_2_practice_progress_val = (UILabel *) ui_get_element_by_tag(s, "practiceprogressvalue_2");

    level_card_coin_1 = (UIImage *) ui_get_element_by_tag(s, "coin_1");
    level_card_coin_2 = (UIImage *) ui_get_element_by_tag(s, "coin_2");
    level_card_coin_3 = (UIImage *) ui_get_element_by_tag(s, "coin_3");
    level_card_2_coin_1 = (UIImage *) ui_get_element_by_tag(s, "coin_1_2");
    level_card_2_coin_2 = (UIImage *) ui_get_element_by_tag(s, "coin_2_2");
    level_card_2_coin_3 = (UIImage *) ui_get_element_by_tag(s, "coin_3_2");

    ui_progress_bar_set_tint(level_card_normal_progress, C2D_Color32(0, 255, 0, 255));
    ui_progress_bar_set_tint(level_card_2_normal_progress, C2D_Color32(0, 255, 0, 255));

    ui_progress_bar_set_tint(level_card_practice_progress, C2D_Color32(0, 255, 255, 255));
    ui_progress_bar_set_tint(level_card_2_practice_progress, C2D_Color32(0, 255, 255, 255));

    update_level_name(curr_level_id, 0);
    update_level_stars(curr_level_id, 0);
    update_level_face(curr_level_id);
    update_level_top(curr_level_id);
    update_level_progress(curr_level_id, 0);

    ui_run_func_on_tag(s, "level_card_2", disable_card_2);

    u32 color = default_lvl_colors[curr_level_id % NUM_MENU_COLORS];
    upload_color_to_buffer(0, color, 0);

    channels[0].color.r = GET_R(color);
    channels[0].color.g = GET_G(color);
    channels[0].color.b = GET_B(color);

    dragging = false;
    dragDistance = 0;
    dragDir = 0;
    cardCorrection = false;

    scroll_dir = 0;
        
    // Set bg color
    bg_gradient = (UIImage *) ui_get_element_by_tag(s, "gradient");

    play_menu_song();
}

void level_select_init_top(UIScreen *s){
    top_screen = s;

    level_card_face  = (UIImage *) ui_get_element_by_tag(s, "level_face");

    bg_gradient_top = (UIImage *) ui_get_element_by_tag(s, "gradient");
}

void level_select_update(UIScreen *s, UIInput *input){
    exiting_level = false;
    if(!scroll_dir){
        upload_color_to_buffer(0, default_lvl_colors[curr_level_id % NUM_MENU_COLORS], 0);
    }

    handle_col_channel(0);

    ColorChannel channel = channels[0];

    ui_image_set_tint(bg_gradient, C2D_Color32(channel.color.r, channel.color.g, channel.color.b, 255));
    ui_image_set_tint(bg_gradient_top, C2D_Color32(channel.color.r, channel.color.g, channel.color.b, 255));
    ui_run_func_on_tag(s, "ground", tint_ground);

    if(cardCorrection){
        ui_run_func_on_tag(s, "level_card_2", disable_card_2);
        ui_set_pos_on_tag(s, 160, LEVEL_CARD_Y_POS, "level_card_2");
        cardCorrection = false;
    }

    if((input->down & KEY_TOUCH)){
        dragStartX = input->touchPosition.px;
        dragging = false;
        dragDir = 0;
        dragDistance = 0;

        recenter();
    }
    if((input->held & KEY_TOUCH)){
        if(dragging){
            int delta = input->touchPosition.px - lastTouchX;
            dragDistance += delta;

            if(dragDistance > 15){
                dragDir = -1;
                peek_left();
            } else if(dragDistance < -15){
                dragDir = 1;
                peek_right();
            } else{
                dragDir = 0;
                ui_run_func_on_tag(s, "level_card_2", disable_card_2);
            }

            ui_set_pos_on_tag(s, 160 + dragDistance, LEVEL_CARD_Y_POS, "level_card");
            ui_set_pos_on_tag(s, 160 + dragDistance + (dragDir * 320), LEVEL_CARD_Y_POS, "level_card_2");
        } else {
            dragDir = 0;

            int dragXDiff = input->touchPosition.px - dragStartX;
            if(abs(dragXDiff) > 10){
                dragging = true;
                scroll_dir = 0;
                anim_duration = 1.f;
                anim_time = anim_duration;
                dragDistance = 0;
                dragDir = 0;
                cardCorrection = true;
                recenter();
            }
        }
        lastTouchX = input->touchPosition.px;
    }
    if((input->up & KEY_TOUCH)){
        if(dragging){
            if(dragDir == -1){
                action_move_left(NULL, NULL);
                anim_duration = 0.5f;
            } else if(dragDir == 1){
                action_move_right(NULL, NULL);
                anim_duration = 0.5f;
            } else{
                scroll_dir = 0;
                anim_time = 0.f;
            }
        }
        dragging = false;
        dragDir = 0;
    }
    
    handle_card_movement();

    //Buttons can't be tapped while dragging, left/right buttons cannot be pressed with keybinds while dragging
    if(dragging){
        input->touchPosition.px = -99;
        input->touchPosition.py = -99;
        ((UIButton *)ui_get_element_by_tag(s, "left"))->keyBinds = 0;
        ((UIButton *)ui_get_element_by_tag(s, "right"))->keyBinds = 0;
    } else{
        ((UIButton *)ui_get_element_by_tag(s, "left"))->keyBinds = (KEY_DLEFT | KEY_L | KEY_ZL | KEY_CPAD_LEFT | KEY_CSTICK_LEFT);
        ((UIButton *)ui_get_element_by_tag(s, "right"))->keyBinds = (KEY_DRIGHT | KEY_R | KEY_ZR | KEY_CPAD_RIGHT | KEY_CSTICK_RIGHT);
    }
}

void level_select_draw_top(UIScreen *s, UIDrawPhase phase){
    if(phase == UI_DRAW_BEFORE) return;

    draw_dots(curr_level_id);
}

const UIScreenDefPair level_select_def = {
    .name = "level_select",
    .top = {
        .path = "romfs:/menus/level_select_top.txt",
        .init = level_select_init_top,
        .draw = level_select_draw_top
    },
    .btm = {
        .path = "romfs:/menus/level_select.txt",
        .init = level_select_init,
        .update = level_select_update,
        .action_list = {
            .action_count = ARRAY_LEN(actions),
            .actions = actions
        }
    }
};
