#include <3ds.h>
#include <citro2d.h>
#include "menus/components/ui_element.h"
#include "menus/components/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_image.h"
#include "fonts/bigFont.h"
#include "main.h"
#include "easing.h"
#include "color_channels.h"
#include "mp3_player.h"
#include "graphics.h"
#include "main_menu.h"
#include "level_select.h"
#include "settings.h"
#include "info_card.h"

#include "save/config.h"

static bool yes_exit = false;

static int current_page = 0;

static UIScreen screen;

bool particlesDisabled = false;
bool wideEnabled = false;
bool glowEnabled = true;
bool yJump = false;
bool touchEffectEverywhere = false;
bool enableDebugBindings = false;
bool hitboxesEnabled = false;
bool hitboxTrail = false;
bool hitboxesOnDeath = false;

static Setting settings[] = {
    {
        "chk_wide", &wideEnabled
    },
    {
        "chk_particle", &particlesDisabled
    },
    {
        "chk_glow", &glowEnabled
    },
    {
        "chk_y_jump", &yJump
    },
    {
        "chk_touch_effect", &touchEffectEverywhere
    },
    {
        "chk_debug_binds", &enableDebugBindings
    },
    {
        "chk_hitbox", &hitboxesEnabled
    },
    {
        "chk_hitboxtrail", &hitboxTrail
    },
    {
        "chk_death_hitboxes", &hitboxesOnDeath
    }
};


#define NUMBER_SETTINGS (sizeof(settings) / sizeof(Setting))


const char *pages_tags[] = {
    "page1",
    "page2",
    "page3"
};

#define NUMBER_PAGES (sizeof(pages_tags) / sizeof(char *))


void switch_page(int page) {
    for (int i = 0; i < NUMBER_PAGES; i++) {
        if (i == page) {
            ui_run_func_on_tag(&screen, pages_tags[page], ui_enable_element);
        } else {
            ui_run_func_on_tag(&screen, pages_tags[i], ui_disable_element);
        }
    }
}

void exit_settings(UIElement* e) {
    yes_exit = true;
}

void wide_settings(UIElement* e) {
    wideEnabled = e->checkbox.checked;
}

void particles_settings(UIElement* e) {
    particlesDisabled = e->checkbox.checked;
}

void glow_settings(UIElement* e) {
    glowEnabled = e->checkbox.checked;
}

void y_button_settings(UIElement* e) {
    yJump = e->checkbox.checked;
}

void touch_effect_settings(UIElement* e) {
    touchEffectEverywhere = e->checkbox.checked;
}

void debug_settings(UIElement* e) {
    enableDebugBindings = e->checkbox.checked;
}

void hitboxes_settings(UIElement* e) {
    hitboxesEnabled = e->checkbox.checked;
}

void hitbox_trail_settings(UIElement* e) {
    hitboxTrail = e->checkbox.checked;
}

void hitboxes_on_death_settings(UIElement* e) {
    hitboxesOnDeath = e->checkbox.checked;
}

void action_left_page(UIElement *e) {
    current_page--;
    if (current_page < 0) {
        current_page = NUMBER_PAGES - 1;
    }

    switch_page(current_page);
}

void action_right_page(UIElement *e) {
    current_page++;
    if (current_page >= NUMBER_PAGES) {
        current_page = 0;
    }

    switch_page(current_page);
}

void action_info_wide(UIElement *e) {
    action_open_info_card(1);
}

void action_info_tap(UIElement *e) {
    action_open_info_card(2);
}

void action_info_jump(UIElement *e) {
    action_open_info_card(3);
}

void action_info_hitboxes(UIElement *e) {
    action_open_info_card(4);
}

void action_info_debug(UIElement *e) {
    action_open_info_card(5);
}

static UIAction actions[] = {
    { "exit", exit_settings },
    { "wide", wide_settings },
    { "particles", particles_settings },
    { "glow", glow_settings },
    { "y_jump", y_button_settings },
    { "touch_effect", touch_effect_settings },
    { "debug", debug_settings },
    { "hitbox", hitboxes_settings },
    { "hitbox_trail", hitbox_trail_settings },
    { "death_hitboxes", hitboxes_on_death_settings },
    { "left_page", action_left_page},
    { "right_page", action_right_page},
    { "wideinfo", action_info_wide},
    { "jumpinfo", action_info_jump},
    { "tapinfo", action_info_tap},
    { "hitboxinfo", action_info_hitboxes},
    { "debuginfo", action_info_debug}
};

void settings_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/settings.txt");
    yes_exit = false;

    for (int i = 0; i < NUMBER_SETTINGS; i++) {
        ui_get_element_by_tag(&screen, settings[i].chk_name)->checkbox.checked = *settings[i].var;
    }

    current_page = 0;

    switch_page(0);
}

int settings_loop() {
    u32 kDown = hidKeysDown();

    if (yes_exit || (kDown & KEY_B)) {
        cfg_save();
        return true;
    }

    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;
    if (!in_info_card) ui_screen_update(&screen, &touch);

    ui_screen_draw(&screen);

    return false;
}