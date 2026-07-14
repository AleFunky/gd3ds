#include <3ds.h>
#include <citro2d.h>
#include "main.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_color_button.h"
#include "menus/components/ui_window_button.h"
#include "graphics.h"
#include "palette_kit.h"
#include "menus/components/ui_checkbox.h"

static bool yes_exit = false;

static UIScreen screen = {
    .isBottom = true
};

const u32 colors[] = {
    // Reds
    ABGR8(253, 212, 206, 255),
    ABGR8(255, 125, 125, 255),
    ABGR8(255, 58, 58, 255),
    ABGR8(255, 0, 0, 255),

    ABGR8(150, 0, 0, 255),
    ABGR8(112, 0, 0, 255),
    ABGR8(82, 2, 0, 255),
    ABGR8(56, 1, 6, 255),

    ABGR8(175, 0, 75, 255),
    ABGR8(128, 79, 79, 255),
    ABGR8(122, 53, 53, 255),
    ABGR8(81, 36, 36, 255),

    // Oranges
    ABGR8(255, 185, 114, 255),
    ABGR8(255, 160, 64, 255),
    ABGR8(255, 125, 0, 255),
    ABGR8(255, 75, 0, 255),

    ABGR8(175, 75, 0, 255),
    ABGR8(163, 98, 70, 255),
    ABGR8(117, 73, 54, 255),
    ABGR8(86, 53, 40, 255),

    ABGR8(150, 50, 0, 255),
    ABGR8(102, 49, 30, 255),
    ABGR8(91, 39, 0, 255),
    ABGR8(71, 32, 0, 255),

    // Yellows
    ABGR8(255, 255, 192, 255),
    ABGR8(255, 250, 127, 255),
    ABGR8(255, 255, 0, 255),
    ABGR8(125, 125, 0, 255),

    ABGR8(253, 224, 160, 255),
    ABGR8(255, 185, 0, 255),
    ABGR8(150, 100, 0, 255),
    ABGR8(80, 50, 14, 255),

    ABGR8(205, 165, 118, 255),
    ABGR8(167, 123, 77, 255),
    ABGR8(109, 83, 57, 255),
    ABGR8(81, 62, 42, 255),

    // Greens
    ABGR8(192, 255, 160, 255),
    ABGR8(177, 255, 109, 255),
    ABGR8(125, 255, 0, 255),
    ABGR8(0, 255, 0, 255), // Default p1

    ABGR8(210, 255, 50, 255),
    ABGR8(75, 175, 0, 255),
    ABGR8(100, 150, 0, 255),
    ABGR8(0, 192, 55, 255),

    ABGR8(0, 255, 125, 255),
    ABGR8(0, 150, 0, 255),
    ABGR8(0, 96, 0, 255),
    ABGR8(0, 64, 0, 255),

    // Cyans
    ABGR8(192, 255, 224, 255),
    ABGR8(148, 255, 228, 255),
    ABGR8(0, 255, 192, 255),
    ABGR8(0, 255, 255, 255), // Default P2, glow

    ABGR8(125, 255, 175, 255),
    ABGR8(67, 161, 138, 255),
    ABGR8(49, 109, 95, 255),
    ABGR8(38, 84, 73, 255),
    
    ABGR8(0, 150, 100, 255),
    ABGR8(0, 125, 125, 255),
    ABGR8(0, 96, 96, 255),
    ABGR8(0, 64, 64, 255),

    // Blues
    ABGR8(160, 255, 255, 255),
    ABGR8(0, 200, 255, 255),
    ABGR8(0, 125, 255, 255),
    ABGR8(0, 0, 255, 255),

    ABGR8(0, 75, 175, 255),
    ABGR8(0, 0, 150, 255),
    ABGR8(1, 7, 112, 255),
    ABGR8(0, 10, 76, 255),

    ABGR8(0, 100, 150, 255),
    ABGR8(0, 73, 109, 255),
    ABGR8(0, 50, 76, 255),
    ABGR8(0, 38, 56, 255),

    ABGR8(118, 189, 255, 255),
    ABGR8(80, 128, 173, 255),
    ABGR8(51, 83, 117, 255),
    ABGR8(35, 60, 86, 255),

    // Purples
    ABGR8(190, 181, 255, 255),
    ABGR8(125, 125, 255, 255),
    ABGR8(125, 0, 255, 255),
    ABGR8(100, 0, 150, 255),

    ABGR8(182, 128, 255, 255),
    ABGR8(75, 0, 175, 255),
    ABGR8(61, 6, 140, 255),
    ABGR8(55, 8, 96, 255),

    ABGR8(77, 77, 143, 255),
    ABGR8(111, 73, 164, 255),
    ABGR8(84, 54, 127, 255),
    ABGR8(66, 42, 99, 255),

    // Magentas
    ABGR8(252, 181, 255, 255),
    ABGR8(255, 0, 125, 255),
    ABGR8(150, 0, 100, 255),
    ABGR8(102, 3, 62, 255),

    ABGR8(255, 0, 255, 255),
    ABGR8(185, 0, 255, 255),
    ABGR8(125, 0, 125, 255),
    ABGR8(71, 1, 52, 255),

    ABGR8(250, 127, 255, 255),
    ABGR8(175, 87, 175, 255),
    ABGR8(130, 67, 130, 255),
    ABGR8(94, 49, 94, 255),

    // Gray scale
    ABGR8(255, 255, 255, 255),
    ABGR8(224, 224, 224, 255),
    ABGR8(175, 175, 175, 255),
    ABGR8(128, 128, 128, 255),
    ABGR8(90, 90, 90, 255),
    ABGR8(64, 64, 64, 255),
    ABGR8(0, 0, 0, 255)
};

const size_t NUM_COLORS = ARRAY_LEN(colors);

/*
Categories:

Red,
Orange,
Yellow,
Green,
Cyan,
Blue,
Purple,
Magenta,
Gray
*/
static int category_colors[] = {
    3,
    15,
    26,
    39,
    51,
    63,
    78,
    89,
    103
};

//amount of colors in each category
static int category_counts[] = {
    12,
    12,
    12,
    12,
    12,
    16,
    12,
    12,
    7
};

static int selected_category;

//the first color index in the selected category
static int starting_color_index;

//used to increment category color buttons during setup
static int category_counter = 0;

//used to increment color buttons during setup
static int color_counter = 0;

//color 1, 2, or glow
static int color_page = 0;

static void disable_glow_setting(UIElement *e) {
    e->enabled = false;
}

static void enable_glow_setting(UIElement *e) {
    e->enabled = true;
}

static void set_color_index(UIElement *e) {
    UIColor *color = (UIColor *) e;

    int color_index = starting_color_index + color_counter;

    ui_color_button_set_index(color, color_page, color_index);

    color->isSelected = *current_colors[color_page] == color_index;

    e->enabled = color_counter < category_counts[selected_category];

    color_counter++;
}

static void set_category_index(UIElement *e) {
    UIColor *color = (UIColor *) e;
    //rather than using the color index as col1/col2/glow, it's used as a category index for the button
    ui_color_button_set_index(color, category_counter, category_colors[category_counter]);
    
    color->isSelected = selected_category == category_counter;
    
    category_counter++;
}

static void reset_indices(){
    category_counter = 0;
    color_counter = 0;

    //get selected category from selected color index
    int current_index = *current_colors[color_page];

    //9 is the number of categories
    for(int i = 0; i < 9; i++){
        if(current_index < category_counts[i]){
            selected_category = i;
            break;
        }
        current_index -= category_counts[i];
    }

    //find first color index
    starting_color_index = 0;
    for(int i = 0; i < selected_category; i++){
        starting_color_index += category_counts[i];
    }

    ui_run_func_on_tag(&screen, "category", set_category_index);
    ui_run_func_on_tag(&screen, "color", set_color_index);
}

static void action_category_selected(UIElement *e){
    UIColor *color = (UIColor *) e;

    //The index of the category color buttons does not correspond to col1/col2/glow, instead corresponding to its category
    *current_colors[color_page] = category_colors[color->index];
    update_player_colors();

    reset_indices();
}

static void action_color_selected(UIElement *e) {
    UIColor *color = (UIColor *) e;

    *current_colors[color_page] = color->color_index;
    update_player_colors();

    reset_indices();
}

void exit_palette_kit(UIElement* e) {
    yes_exit = true;
}

static void disable_all_color_buttons(UIElement *e) {
    ui_window_button_set_style((UIWindowButton *) e, 4);
}

static void set_p1_page(UIElement *e) {    
    ui_run_func_on_tag(&screen, "color_buttons", disable_all_color_buttons);
    ui_window_button_set_style((UIWindowButton *) e, 5);
    color_page = 0;
    ui_run_func_on_tag(&screen, "glow_option", disable_glow_setting);
    reset_indices();
}

static void set_p2_page(UIElement *e) {
    ui_run_func_on_tag(&screen, "color_buttons", disable_all_color_buttons);
    ui_window_button_set_style((UIWindowButton *) e, 5);
    color_page = 1;
    ui_run_func_on_tag(&screen, "glow_option", disable_glow_setting);
    reset_indices();
}

static void set_glow_page(UIElement *e) {
    ui_run_func_on_tag(&screen, "color_buttons", disable_all_color_buttons);
    ui_window_button_set_style((UIWindowButton *) e, 5);
    color_page = 2;
    ui_run_func_on_tag(&screen, "glow_option", enable_glow_setting);
    reset_indices();
}

void player_glow_settings(UIElement* e) {
    UICheckBox *checkbox = (UICheckBox *) e;
    player_glow_enabled = checkbox->checked;
}

static UIAction actions[] = {
    {"exit", exit_palette_kit},
    {"color_selected", action_color_selected},
    {"action_p1", set_p1_page},
    {"action_p2", set_p2_page},
    {"action_glow", set_glow_page},
    {"glow", player_glow_settings},
    {"category_selected", action_category_selected}
};


void palette_kit_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/palette_kit.txt");
    ui_screen_open(&screen, ANIM_SLIDE_RIGHT);
    yes_exit = false;
    ui_window_set_tint((UIWindow *) ui_get_element_by_tag(&screen, "bg_window"), C2D_Color32(0, 0, 0, 64));
    ui_window_set_tint((UIWindow *) ui_get_element_by_tag(&screen, "color_window"), C2D_Color32(0, 0, 0, 64));

    color_page = 0;

    reset_indices();

    set_checkbox_enabled((UICheckBox *) ui_get_element_by_tag(&screen, "check_glow"), player_glow_enabled);
    ui_run_func_on_tag(&screen, "glow_option", disable_glow_setting);
}

int palette_kit_loop() {
    if (yes_exit) {  
        ui_unload_screen(&screen);
        return true;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    ui_screen_update(&screen, &touch);
    ui_screen_draw(&screen);

    return false;
}