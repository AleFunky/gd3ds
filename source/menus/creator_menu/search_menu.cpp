#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "main.hpp"
#include "graphics.hpp"

#include "menus/core/ui_element.hpp"
#include "utils/server_utils.hpp"

#include "save/saving.hpp"
#include "save/config.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/creator_menu/online/online_menu.hpp"
#include "menus/creator_menu/search/server_switcher.hpp"
#include "menus/creator_menu/search/search_filters.hpp"
#include "menus/creator_menu/search/clear_search_filters.hpp"
#include "menus/creator_menu/search/song_filter.hpp"

bool search_needs_refresh = true;
bool gdps = false;

bool user_coins_counter_visible(void) {
    return !gdps;
}

static UIScreen *btm_screen = NULL;

static void update_difficulty_tint(UIElement *e){
    int tint = (filters.difficultyFilters & (ui_prop_int(&e->custom_properties, "diffValue", 0))) > 0 ? 255 : 127;
    C2D_PlainImageTint(&((UIButton *)e)->image.tint, C2D_Color32(tint, tint, tint, 255), 1.f);
}

void enable_demons(UIScreen *s){
    if(gdps) return;

    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "easy")), 259, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "normal")), 261, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "hard")), 257, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "harder")), 263, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "insane")), 265, 0);
}

void disable_demons(UIScreen *s){
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "easy")), 252, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "normal")), 253, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "hard")), 254, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "harder")), 255, 0);
    ui_button_set_image(((UIButton *)ui_get_element_by_tag(s, "insane")), 256, 0);
}

void update_difficulty_tints(UIScreen *s){
    C2D_PlainImageTint(
        &((UIButton *)ui_get_element_by_tag(s, "na"))->image.tint, 
        filters.isNA ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    C2D_PlainImageTint(
        &((UIButton *)ui_get_element_by_tag(s, "auto"))->image.tint, 
        filters.isAuto ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    C2D_PlainImageTint(
        &((UIButton *)ui_get_element_by_tag(s, "demon"))->image.tint, 
        filters.isDemon ? C2D_Color32(255, 255, 255, 255) : C2D_Color32(127, 127, 127, 255), 
        1.f);
    ui_run_func_on_tag(s, "difficulty", update_difficulty_tint);
}

static void action_na(UIElement *e, const UIPropertyList *args){
    filters.isNA = !filters.isNA;
    if(filters.isNA){
        filters.isAuto = false;
        filters.isDemon = false;
        disable_demons(e->screen);
        filters.difficultyFilters = 0;
    }
    update_difficulty_tints(e->screen);
}

static void action_auto(UIElement *e, const UIPropertyList *args){
    filters.isAuto = !filters.isAuto;
    if(filters.isAuto){
        filters.isNA = false;
        filters.isDemon = false;
        disable_demons(e->screen);
        filters.difficultyFilters = 0;
    }
    update_difficulty_tints(e->screen);
}

static void action_demon(UIElement *e, const UIPropertyList *args){
    filters.isDemon = !filters.isDemon;
    if(filters.isDemon){
        filters.isNA = false;
        filters.isAuto = false;
        filters.difficultyFilters = 0;
        enable_demons(e->screen);
    } else{
        filters.difficultyFilters = 0;
        disable_demons(e->screen);
    }
    update_difficulty_tints(e->screen);
}

static void action_set_difficulty(UIElement *e, const UIPropertyList *args){
    filters.isNA = false;
    filters.isAuto = false;
    int difficultyVal = ui_prop_int(&e->custom_properties, "diffValue", 0);

    if(filters.isDemon && !gdps) filters.difficultyFilters &= difficultyVal;
    else filters.isDemon = false;

    filters.difficultyFilters ^= difficultyVal;
    update_difficulty_tints(e->screen);
}


void action_set_query(UIElement* e, const UIPropertyList *args){
    snprintf(filters.searchQuery, sizeof(filters.searchQuery), "%.*s", (int)sizeof(filters.searchQuery) - 1, ((UITextbox *)e)->text);
}

static void action_clear_search(UIElement* e, const UIPropertyList *args){
    UITextbox *searchBox = ((UITextbox *)ui_get_element_by_tag(e->screen, "searchbox"));
    if (!searchBox) return;

    searchBox->text[0] = '\0';
    filters.searchQuery[0] = '\0';
}

static void action_search(UIElement* e, const UIPropertyList *args) {
    filters.searchType = ui_prop_int(&e->custom_properties, "type", 0);
    filters.currentPage = 0;
    search_needs_refresh = true;
    ui_stack_push(&online_def, ANIM_NONE, ANIM_NONE, PUSH_ROOT);
}

static UIActionDef search_actions[] = {
    {"search", action_search },
    {"clearsearch", action_clear_search },
    {"searchtext", action_set_query},
    {"difficulty", action_set_difficulty },
    {"na", action_na },
    {"auto", action_auto },
    {"demon", action_demon },
};

static void search_menu_init(UIScreen *s) {
    btm_screen = s;

    UITextbox *searchBox = ((UITextbox *)ui_get_element_by_tag(s, "searchbox"));
    snprintf(searchBox->text, sizeof(searchBox->text), "%s", filters.searchQuery);

    update_difficulty_tints(s);
    if(filters.isDemon) enable_demons(s);
}

static void search_menu_exit(UIScreen *s) {
    cfg_save();
}

extern const UIScreenDefPair search_menu_def = {
    .name = "search_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/search_menu_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/search_menu.txt",
        .init = search_menu_init,
        .exit = search_menu_exit,
        .action_list = {
            .actions = search_actions,
            .action_count = ARRAY_LEN(search_actions)
        }
    }
};
