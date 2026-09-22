#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_icon.h"
#include "menus/components/ui_label.h"
#include "save/saving.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "icon_kit.h"
#include "level/main_levels.h"

#include "palette_kit.h"

#include "save/config.h"
#include "state.h"

static int gamemode_page = 0;

// Page handling 

static int current_cube_page = 0;
static int current_ship_page = 0;
static int current_ball_page = 0;
static int current_ufo_page  = 0;
static int current_wave_page = 0;
static int current_trail_page = 0;

static int last_displayed_gamemode = 0;

int selected_cube = 1;
int selected_ship = 1;
int selected_ball = 1;
int selected_ufo  = 1;
int selected_wave = 1;
int selected_trail = 1;

int selected_p1 = 0;
int selected_p2 = 0;
int selected_glow = 0;

bool player_glow_enabled = false;
bool show_glow = false;

const int gamemode_icon_count[GAMEMODE_COUNT + 1] = {
    ICON_COUNT_PLAYER,
    ICON_COUNT_SHIP,
    ICON_COUNT_PLAYER_BALL,
    ICON_COUNT_BIRD,
    ICON_COUNT_DART,
    TRAIL_COUNT
};

static int *current_pages[GAMEMODE_COUNT + 1] = {
    &current_cube_page,
    &current_ship_page,
    &current_ball_page,
    &current_ufo_page,
    &current_wave_page,
    &current_trail_page
};

static UIButton *gamemode_btns[GAMEMODE_COUNT + 1];

int *current_icons[GAMEMODE_COUNT + 1] = {
    &selected_cube,
    &selected_ship,
    &selected_ball,
    &selected_ufo,
    &selected_wave,
    &selected_trail
};

int *current_colors[3] = {
    &selected_p1,
    &selected_p2,
    &selected_glow
};

static const int button_images[6] = {
    341,
    351,
    325,
    327,
    329,
    355
};
static int icon_counter = 1;

static void set_icon_index(UIElement *e) {
    int new_index = (*current_pages[gamemode_page] * ICONS_PER_PAGE) + icon_counter;
    if (new_index < gamemode_icon_count[gamemode_page]) {
        e->enabled = true;
        ui_icon_set_selected((UIIcon *) e, *current_icons[gamemode_page] == new_index);
        ui_icon_set_gamemode_index((UIIcon *) e, gamemode_page, new_index);

        if (gamemode_page == GAMEMODE_SHIP || gamemode_page == GAMEMODE_UFO) {
            ui_element_set_scale(e, 0.6f);
        } else {
            ui_element_set_scale(e, 0.7f);
        }

        icon_counter++;
    } else {
        e->enabled = false;
    }
}

static void set_trail_index(UIElement *e) {
    int new_index = icon_counter;
    if (new_index < gamemode_icon_count[gamemode_page]) {
        e->enabled = true;
        ui_icon_set_selected((UIIcon *) e, *current_icons[gamemode_page] == new_index);
        ui_icon_set_gamemode_index((UIIcon *) e, gamemode_page, new_index);
        icon_counter++;
    } else {
        e->enabled = false;
    }
}

static void disable_all_icon_buttons(UIScreen *s) {
    icon_counter = (gamemode_page == TRAIL ? 0 : 1);
    ui_button_set_image((UIButton *) ui_get_element_by_tag(s, "cube"), button_images[0], 0);
    ui_button_set_image((UIButton *) ui_get_element_by_tag(s, "ship"), button_images[1], 0);
    ui_button_set_image((UIButton *) ui_get_element_by_tag(s, "ball"), button_images[2], 0);
    ui_button_set_image((UIButton *) ui_get_element_by_tag(s, "ufo"),  button_images[3], 0);
    ui_button_set_image((UIButton *) ui_get_element_by_tag(s, "dart"), button_images[4], 0);
    ui_button_set_image((UIButton *) ui_get_element_by_tag(s, "trail"), button_images[5], 0);
}

static void action_set_page(UIElement *e, const UIPropertyList *args) {
    gamemode_page = ui_prop_int(args, "page", 0);
    last_displayed_gamemode = gamemode_page;
    disable_all_icon_buttons(e->screen);
    ui_button_set_image((UIButton *) e, button_images[gamemode_page] + 1, 0);
    ui_run_func_on_tag(e->screen, "icon", set_icon_index); 

    for(int i = 0; i < GAMEMODE_COUNT + 1; i++){
        gamemode_btns[i]->keyBinds = 0;
        if(i == gamemode_page - 1){
            gamemode_btns[i]->keyBinds |= KEY_L | KEY_ZL;
        } else if(i == gamemode_page + 1){
            gamemode_btns[i]->keyBinds |= KEY_R | KEY_ZR;
        } 
    }
}

static void set_trail_page(UIElement *e, const UIPropertyList *args) {
    gamemode_page = 5;
    disable_all_icon_buttons(e->screen);
    ui_button_set_image((UIButton *) e, button_images[5] + 1, 0);
    ui_run_func_on_tag(e->screen, "icon", set_trail_index); 
}


static void move_index_left(UIElement* e, const UIPropertyList *args) {
    *current_pages[gamemode_page] -= 1;
    if (*current_pages[gamemode_page] < 0) {
        *current_pages[gamemode_page] = (gamemode_icon_count[gamemode_page] - 2) / ICONS_PER_PAGE;
    }
    icon_counter = (gamemode_page == TRAIL ? 0 : 1);
    ui_run_func_on_tag(e->screen, "icon", set_icon_index); 
}

static void move_index_right(UIElement* e, const UIPropertyList *args) {
    *current_pages[gamemode_page] += 1;
    if ((*current_pages[gamemode_page] * ICONS_PER_PAGE) + 1 >= gamemode_icon_count[gamemode_page]) {
        *current_pages[gamemode_page] = 0;
    }
    icon_counter = (gamemode_page == TRAIL ? 0 : 1);
    ui_run_func_on_tag(e->screen, "icon", set_icon_index); 
}

static void action_icon_selected(UIElement *e, const UIPropertyList *args) {
    *current_icons[((UIIcon *) e)->gamemode] = ((UIIcon *) e)->index;
    icon_counter = (gamemode_page == TRAIL ? 0 : 1);
    ui_run_func_on_tag(e->screen, "icon", set_icon_index); 
}

static void icon_kit_init(UIScreen *s){
    gamemode_btns[0] = (UIButton *) ui_get_element_by_tag(s, "cube");
    gamemode_btns[1] = (UIButton *) ui_get_element_by_tag(s, "ship");
    gamemode_btns[2] = (UIButton *) ui_get_element_by_tag(s, "ball");
    gamemode_btns[3] = (UIButton *) ui_get_element_by_tag(s, "ufo");
    gamemode_btns[4] = (UIButton *) ui_get_element_by_tag(s, "dart");
    gamemode_btns[5] = (UIButton *) ui_get_element_by_tag(s, "trail");

    action_set_page(ui_get_element_by_tag(s, "cube"), NULL);

    update_player_colors();
    show_glow = (player_glow_enabled || ((p1_color.r | p1_color.g | p1_color.b) == 0));

    play_menu_song();
}

static void icon_kit_init_top(UIScreen *s){
    char stars[32];
    snprintf(stars, sizeof(stars), "%d", total_stars);
    
    char coins[32];
    snprintf(coins, sizeof(coins), "%d", total_coins);

    ui_label_set_text((UILabel *) ui_get_element_by_tag(s, "star_text"), stars);
    ui_label_set_text((UILabel *) ui_get_element_by_tag(s, "secretcoins_text"), coins);

}

static void icon_kit_update_top(UIScreen *s, UIInput *input){
    UIIcon *icon = (UIIcon *)ui_get_element_by_tag(s, "big_icon");
 
    ui_icon_set_gamemode_index(icon, last_displayed_gamemode, *current_icons[last_displayed_gamemode]);
    ui_icon_set_p1(icon, C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255));
    ui_icon_set_p2(icon, C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255));
    ui_icon_set_glow(icon, C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255));
    icon->glow = show_glow;
}

static const UIActionDef icon_kit_actions[] = {
    {"action_page", action_set_page },
    {"action_trail", set_trail_page },
    {"icons_left", move_index_left },
    {"icons_right", move_index_right },
    {"icon_selected", action_icon_selected }
};

const UIScreenDefPair icon_kit_def = {
    .name = "icon_kit",
    .top = {
        .path = "romfs:/menus/icon_kit_top.txt",
        .init = icon_kit_init_top,
        .update = icon_kit_update_top
    },
    .btm = {
        .path = "romfs:/menus/icon_kit.txt",
        .init = icon_kit_init,
        .action_list = {
            .action_count = ARRAY_LEN(icon_kit_actions),
            .actions = icon_kit_actions
        }
    }
};