#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_list.h"
#include "main_menu.h"
#include "settings.h"

#include "mp3_player.h"
#include "save/config.h"
#include "state.h"

static bool yes_exit = false;

static int current_page = 0;

static UIScreen screen = {
    .isBottom = true
};

bool particlesDisabled = false;
bool wideEnabled = false;
bool stereoEnabled = true;
bool glowEnabled = true;
bool yJump = false;
bool touchEffectEverywhere = false;
bool enableDebugBindings = false;
bool hitboxesEnabled = false;
bool hitboxTrail = false;
bool hitboxesOnDeath = false;
bool showProgressBar = false;
bool showProgressPercent = false;
bool decimalPercent = false;
bool ultraDecimalPercent = false;
bool switchTrailColor = false;
bool switchWaveTrailColor = false;
bool quickRetry = false;
bool solidWaveTrail = false;
bool noPlayerTrail = false;
bool noWaveTrailBehind = false;
bool doNot = false;
bool practiceMusicSync = false;

static Setting settings[] = {
    {
        "chk_wide", &wideEnabled
    },
    {
        "chk_stereo", &stereoEnabled
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
    },
    {
        "chk_progressbar", &showProgressBar
    },
    {
        "chk_progresspercent", &showProgressPercent
    },
    {
        "chk_decimalpercent", &decimalPercent
    },
    {
        "chk_ultradecimalpercent", &ultraDecimalPercent
    },
    {
        "chk_trailcolor", &switchTrailColor
    },
    {
        "chk_wavetrailcolor", &switchWaveTrailColor
    },
    {
        "chk_quickretry", &quickRetry
    },
    {
        "chk_solidwavetrail", &solidWaveTrail
    },
    {
        "chk_noplayertrail", &noPlayerTrail
    },
    {
        "chk_nowavetrailbehind", &noWaveTrailBehind
    },
    {
        "chk_donot", &doNot
    },
    {
        "chk_practicemusicsync", &practiceMusicSync
    }
};


const char *pages_tags[] = {
    "page1",
    "page2",
    "page3",
    "page4",
    "page5",
    "page6",
    "page7",
};


void switch_page(int page) {
    for (int i = 0; i < ARRAY_LEN(pages_tags); i++) {
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

// Wide and 3D both want the whole top screen, so only one of them gets it
static void turn_off_setting(const char *chk_name, bool *var) {
    *var = false;
    set_checkbox_enabled((UICheckBox *)ui_get_element_by_tag(&screen, chk_name), false);
}

void wide_settings(UIElement* e) {
    wideEnabled = ((UICheckBox *)e)->checked;

    if (wideEnabled) turn_off_setting("chk_stereo", &stereoEnabled);
}

void stereo_settings(UIElement* e) {
    stereoEnabled = ((UICheckBox *)e)->checked;

    if (stereoEnabled) turn_off_setting("chk_wide", &wideEnabled);
}

void particles_settings(UIElement* e) {
    particlesDisabled = ((UICheckBox *)e)->checked;
}

void glow_settings(UIElement* e) {
    glowEnabled = ((UICheckBox *)e)->checked;
}

void y_button_settings(UIElement* e) {
    yJump = ((UICheckBox *)e)->checked;
}

void touch_effect_settings(UIElement* e) {
    touchEffectEverywhere = ((UICheckBox *)e)->checked;
}

void debug_settings(UIElement* e) {
    enableDebugBindings = ((UICheckBox *)e)->checked;
}

void hitboxes_settings(UIElement* e) {
    hitboxesEnabled = ((UICheckBox *)e)->checked;
}

void hitbox_trail_settings(UIElement* e) {
    hitboxTrail = ((UICheckBox *)e)->checked;
}

void hitboxes_on_death_settings(UIElement* e) {
    hitboxesOnDeath = ((UICheckBox *)e)->checked;
}

void progressbar_settings(UIElement* e) {
    showProgressBar = ((UICheckBox *)e)->checked;
}

void progresspercent_settings(UIElement* e) {
    showProgressPercent = ((UICheckBox *)e)->checked;
}

void decimalpercent_settings(UIElement* e) {
    decimalPercent = ((UICheckBox *)e)->checked;
}

void ultradecimalpercent_settings(UIElement* e) {
    ultraDecimalPercent = ((UICheckBox *)e)->checked;
}

void switchTrailColor_settings(UIElement* e) {
    switchTrailColor = ((UICheckBox *)e)->checked;
}

void switchWaveTrailColor_settings(UIElement* e) {
    switchWaveTrailColor = ((UICheckBox *)e)->checked;
}

void quickRetry_settings(UIElement* e) {
    quickRetry = ((UICheckBox *)e)->checked;
}

void solidWaveTrail_settings(UIElement* e) {
    solidWaveTrail = ((UICheckBox *)e)->checked;
}

void noPlayerTrail_settings(UIElement* e) {
    noPlayerTrail = ((UICheckBox *)e)->checked;
}

void noWaveTrailBehind_settings(UIElement* e) {
    noWaveTrailBehind = ((UICheckBox *)e)->checked;
}

void doNot_settings(UIElement* e) {
    doNot = ((UICheckBox *)e)->checked;
}

void practiceMusicSync_settings(UIElement* e) {
    practiceMusicSync = ((UICheckBox *)e)->checked;

    // Enable song
    if (state.practice_mode) {
        stop_mp3();
        if (practiceMusicSync) {
            play_level_song(level_info.song_offset + state.player.timeElapsed);
        } else {
            play_practice_song();
        }
    }
}

void action_left_page(UIElement *e) {
    current_page--;
    if (current_page < 0) {
        current_page = ARRAY_LEN(pages_tags) - 1;
    }

    switch_page(current_page);
}

void action_right_page(UIElement *e) {
    current_page++;
    if (current_page >= ARRAY_LEN(pages_tags)) {
        current_page = 0;
    }

    switch_page(current_page);
}

void action_info_wide(UIElement *e) {
    action_open_info_card(1);
}

void action_info_stereo(UIElement *e) {
    action_open_info_card(14);
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

void action_info_decimal(UIElement *e) {
    action_open_info_card(6);
}

void action_info_ultra_decimal(UIElement *e) {
    action_open_info_card(7);
}

void action_info_trail(UIElement *e) {
    action_open_info_card(8);
}

void action_info_wave_trail(UIElement *e) {
    action_open_info_card(9);
}

void action_info_quick_retry(UIElement *e) {
    action_open_info_card(10);
}

void action_info_solid_wave_trail(UIElement *e) {
    action_open_info_card(11);
}

void action_info_no_wave_trail_behind(UIElement *e) {
    action_open_info_card(12);
}

void action_info_do_not(UIElement *e) {
    action_open_info_card(13);
}


static UIAction actions[] = {
    { "exit", exit_settings },
    { "wide", wide_settings },
    { "stereo", stereo_settings },
    { "particles", particles_settings },
    { "glow", glow_settings },
    { "y_jump", y_button_settings },
    { "touch_effect", touch_effect_settings },
    { "debug", debug_settings },
    { "hitbox", hitboxes_settings },
    { "hitbox_trail", hitbox_trail_settings },
    { "death_hitboxes", hitboxes_on_death_settings },
    { "progressbar", progressbar_settings },
    { "progresspercent", progresspercent_settings },
    { "decimalPercent", decimalpercent_settings },
    { "ultraDecimalPercent", ultradecimalpercent_settings },
    { "switchTrailColor", switchTrailColor_settings},
    { "switchWaveTrailColor", switchWaveTrailColor_settings},
    { "quickRetry", quickRetry_settings},
    { "solidWaveTrail", solidWaveTrail_settings},
    { "noPlayerTrail", noPlayerTrail_settings},
    { "noWaveTrailBehind", noWaveTrailBehind_settings},
    { "doNot", doNot_settings},
    { "practiceMusicSync" , practiceMusicSync_settings},
    { "left_page", action_left_page},
    { "right_page", action_right_page},
    { "wideinfo", action_info_wide},
    { "stereoinfo", action_info_stereo},
    { "jumpinfo", action_info_jump},
    { "tapinfo", action_info_tap},
    { "hitboxinfo", action_info_hitboxes},
    { "debuginfo", action_info_debug},
    { "decimalinfo", action_info_decimal},
    { "ultradecimalinfo", action_info_ultra_decimal},
    { "trailcolorinfo", action_info_trail},
    { "wavetrailcolorinfo", action_info_wave_trail},
    { "quickretryinfo", action_info_quick_retry},
    { "solidwavetrailinfo", action_info_solid_wave_trail},
    { "nowavetrailbehindinfo", action_info_no_wave_trail_behind},
    { "donotinfo", action_info_do_not}
};

void settings_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/settings.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    yes_exit = false;

    for (int i = 0; i < ARRAY_LEN(settings); i++) {
        UICheckBox *checkbox = (UICheckBox *)ui_get_element_by_tag(&screen, settings[i].chk_name);
        if (checkbox) {
            checkbox->checked = *settings[i].var;
            set_checkbox_enabled(checkbox, checkbox->checked);
        }
    }

    current_page = 0;

    switch_page(0);
}

int settings_loop() {
    if (yes_exit) {
        cfg_save();
        
        ui_unload_screen(&screen);
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