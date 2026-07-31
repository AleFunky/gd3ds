#include <3ds.h>
#include <citro2d.h>
#include "main.h"
#include "menus/components/ui_button.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_rectangle.h"
#include "menus/components/ui_window_button.h"
#include "menus/core/common_setters.h"
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_list.h"
#include "main_menu.h"
#include "settings.h"

#include "mp3_player.h"
#include "save/config.h"
#include "state.h"
#include "utils/gfx.h"
#include "utils/json_config.h"
#include "utils/precise_input.h"

static bool yes_exit = false;

static int current_page = 0;

static UIScreen screen = {
    .isBottom = true
};

SettingState settingsState;

UIList *list;
UILabel *category_name;

bool request_list_reload = false;

void wide_setting(bool checked);
void stereo_setting(bool checked);
void practiceMusicSync_setting(bool checked);

const char *category_names[] = {
    "Graphics",
    "Input",
    "Misc",
    "Gameplay",
    "Cosmetic"
};

Setting settings[] = {
    {
        .id = "wideEnabled",
        .label = "800 px res",
        .additionalInfo = "Doubles the top screen's horizontal<p>resolution.",
        .page = PAGE_GRAPHICS, 

        .defaultValue = false,
        .var = &settingsState.wideEnabled,
        .key = CONFIG_GRAPHICS_PATH "wideEnabled",

        .onChanged = wide_setting
    },
    {
        .id = "stereoEnabled",
        .label = "Stereoscopic 3D",
        .additionalInfo = "Adds depth to the top screen.<p>Use the 3D slider, costs some FPS.",
        .page = PAGE_GRAPHICS, 

        .defaultValue = false,
        .var = &settingsState.stereoEnabled,
        .key = CONFIG_GRAPHICS_PATH "stereoEnabled",

        .onChanged = stereo_setting
    },
    {
        .id = "particlesDisabled",
        .label = "Disable particles",
        .additionalInfo = NULL,
        .page = PAGE_GRAPHICS, 

        .defaultValue = false,
        .var = &settingsState.particlesDisabled,
        .key = CONFIG_GRAPHICS_PATH "particlesDisabled"
    },
    {
        .id = "glowEnabled",
        .label = "Enable object glow",
        .additionalInfo = NULL,
        .page = PAGE_GRAPHICS, 

        .defaultValue = true,
        .var = &settingsState.glowEnabled,
        .key = CONFIG_GRAPHICS_PATH "glowEnabled"
    },
    {
        .id = "yButton",
        .label = "Y to jump",
        .additionalInfo = "Swaps your jump input to Y.",
        .page = PAGE_INPUT, 

        .defaultValue = false,
        .var = &settingsState.yJump,
        .key = CONFIG_INPUT_PATH "yButton"
    },
    {
        .id = "touchEffectEverywhere",
        .label = "Global tap effect",
        .additionalInfo = "Plays the tap effect across all menus.",
        .page = PAGE_INPUT, 

        .defaultValue = false,
        .var = &settingsState.touchEffectEverywhere,
        .key = CONFIG_INPUT_PATH "touchEffectEverywhere"
    },
    {
        .id = "enableDebugBindings",
        .label = "Enable debug keys",
        .additionalInfo = "Enables debug key shortcuts.<p>(B + L, B + R, X)",
        .page = PAGE_INPUT,

        .defaultValue = false,
        .var = &settingsState.enableDebugBindings,
        .key = CONFIG_INPUT_PATH "enableDebugBindings"
    },
    {
        .id = "preciseInput",
        .label = "240Hz input (CBF)",
        .additionalInfo = "Registers jumps on the exact 240Hz<p>physics tick they were pressed on.",
        .page = PAGE_INPUT,

        .defaultValue = false,
        .var = &pi_enabled,
        .key = CONFIG_INPUT_PATH "preciseInput"
    },
    {
        .id = "hitboxesEnabled",
        .label = "Show hitboxes",
        .additionalInfo = "Shows object hitboxes while in a level.<p>WARNING: AFFECTS PERFOMANCE!",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.hitboxesEnabled,
        .key = CONFIG_MISC_PATH "hitboxesEnabled"
    },
    {
        .id = "hitboxTrail",
        .label = "Enable hitbox trail",
        .additionalInfo = NULL,
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.hitboxTrail,
        .key = CONFIG_MISC_PATH "hitboxTrail"
    },
    {
        .id = "hitboxesOnDeath",
        .label = "Show hitboxes<p>on death",
        .additionalInfo = NULL,
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.hitboxesOnDeath,
        .key = CONFIG_MISC_PATH "hitboxesOnDeath"
    },
    {
        .id = "doNot",
        .label = "Do not",
        .additionalInfo = "Doesn't do anything...<p>Well, nothing useful.",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.doNot,
        .key = CONFIG_MISC_PATH "doNot"
    },
    {
        .id = "practiceMusicSync",
        .label = "Practice music sync",
        .additionalInfo = "Plays the level's music in practice mode.",
        .page = PAGE_MISC, 

        .defaultValue = false,
        .var = &settingsState.practiceMusicSync,
        .key = CONFIG_MISC_PATH "practiceMusicSync",

        .onChanged = practiceMusicSync_setting
    },
    {
        .id = "showProgressBar",
        .label = "Show progress bar",
        .additionalInfo = NULL,
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.showProgressBar,
        .key = CONFIG_GAMEPLAY_PATH "showProgressBar"
    },
    {
        .id = "showProgressPercent",
        .label = "Show level percentage",
        .additionalInfo = NULL,
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.showProgressPercent,
        .key = CONFIG_GAMEPLAY_PATH "showProgressPercent"
    },
    {
        .id = "decimalPercent",
        .label = "Accurate percentage",
        .additionalInfo = "Shows level progress with 2 decimals.",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.decimalPercent,
        .key = CONFIG_GAMEPLAY_PATH "decimalPercent"
    },
    {
        .id = "ultraDecimalPercent",
        .label = "Ultra percentage",
        .additionalInfo = "But mom, I want more decimals!!!!<p>(Why would you want to use this?)",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.ultraDecimalPercent,
        .key = CONFIG_GAMEPLAY_PATH "ultraDecimalPercent"
    },
    {
        .id = "quickRetry",
        .label = "Quick retry",
        .additionalInfo = "Restarts in 0.5 seconds instead of 1.",
        .page = PAGE_GAMEPLAY, 

        .defaultValue = false,
        .var = &settingsState.quickRetry,
        .key = CONFIG_GAMEPLAY_PATH "quickRetry"
    },
    {
        .id = "switchTrailColor",
        .label = "Switch trail color",
        .additionalInfo = "Makes the player trail use P1<p>instead of P2.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.switchTrailColor,
        .key = CONFIG_COSMETIC_PATH "switchTrailColor"
    },
    {
        .id = "switchWaveTrailColor",
        .label = "Switch wave<p>trail color",
        .additionalInfo = "Makes the wave trail use P1<p>instead of P2.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.switchWaveTrailColor,
        .key = CONFIG_COSMETIC_PATH "switchWaveTrailColor"
    },
    {
        .id = "solidWaveTrail",
        .label = "Solid wave trail",
        .additionalInfo = "Disables blending for the wave trail.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.solidWaveTrail,
        .key = CONFIG_COSMETIC_PATH "solidWaveTrail"
    },
    {
        .id = "noPlayerTrail",
        .label = "Disable player trail",
        .additionalInfo = NULL,
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.noPlayerTrail,
        .key = CONFIG_COSMETIC_PATH "noPlayerTrail"
    },
    {
        .id = "noWaveTrailBehind",
        .label = "No wave trail behind",
        .additionalInfo = "Disables player trail for the wave.",
        .page = PAGE_COSMETIC, 

        .defaultValue = false,
        .var = &settingsState.noWaveTrailBehind,
        .key = CONFIG_COSMETIC_PATH "noWaveTrailBehind"
    },
};

typedef struct CheckboxData {
    Setting *setting;
} CheckboxData;

typedef struct InfoButtonData {
    const char *text;
} InfoButtonData;

void exit_settings(UIElement* e) {
    yes_exit = true;
}

UICheckBox *get_setting_checkbox_by_id(const char *id) {
    if (list) {
        // Iterate through list children
        for (UIElement *rectangle = list->base.first_child; rectangle; rectangle = rectangle->next_sibling) {
            // Find checkbox in the rectangle
            UIElement *checkbox = ui_get_child_by_type(rectangle, UI_CHECKBOX);
            if (checkbox) {
                if (checkbox->userdata) {
                    CheckboxData *data = checkbox->userdata;

                    // Check if the ids match
                    if (strcmp(id, data->setting->id) == 0) {
                        return (UICheckBox *) checkbox;
                    }
                }
            }
        }
    }

    return NULL;
}

// Wide and 3D both want the whole top screen, so only one of them gets it
static void turn_off_setting(const char *id, bool *var) {
    *var = false;

    UICheckBox *checkbox = get_setting_checkbox_by_id(id);

    if (checkbox) ui_set_checkbox_checked(checkbox, false);
}

void wide_setting(bool checked) {
    if (checked) turn_off_setting("stereoEnabled", &settingsState.stereoEnabled);
}

void stereo_setting(bool checked) {
    if (!checked) return;

    // No 3D on this console, so don't let the box stay ticked
    if (!stereo_supported()) {
        turn_off_setting("stereoEnabled", &settingsState.stereoEnabled);
        return;
    }

    turn_off_setting("wideEnabled", &settingsState.wideEnabled);
}

void practiceMusicSync_setting(bool checked) {
    // Enable song
    if (state.practice_mode) {
        stop_mp3();
        if (checked) {
            play_level_song(level_info.song_offset + state.player.timeElapsed);
        } else {
            play_practice_song();
        }
    }
}

static void checkbox_action(UIElement *e) {
    CheckboxData *data = e->userdata;
    bool checked = ((UICheckBox *)e)->checked;
    if (data) {
        *data->setting->var = checked;
        if (data->setting->onChanged) data->setting->onChanged(checked);
    }
}

static void info_action(UIElement *e) {
    InfoButtonData *data = e->userdata;
    if (data) {
        action_open_info_card_text(data->text);
    }
}

static void set_button_style(UIElement *e) {
    UIWindowButton *button = (UIWindowButton *) e;
    int page = ui_prop_int(&e->custom_properties, "page", 0);
    ui_window_button_set_style(button, (page == current_page ? 10 : 5));
}

void action_category(UIElement *e) {
    current_page = ui_prop_int(&e->custom_properties, "page", 0);
    request_list_reload = true;
}


void create_setting(Setting *setting, int id) {
    if (list) {
        float list_width = list->base.w * 0.5f;

        UIElement *card = (UIElement *) ui_create_rectangle(&screen.ctx);

        if (card) {
            ui_rectangle_set_color((UIRectangle *) card, (id & 1 ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));
            ui_element_set_size(card, 0, 28);

            UICheckBox *checkbox = ui_create_checkbox(&screen.ctx);
            if (checkbox) {
                // Store in the user data
                CheckboxData *data = malloc(sizeof(*data));
                data->setting = setting;

                ui_element_set_userdata((UIElement *) checkbox, data);

                ui_element_set_position((UIElement *) checkbox, -list_width + 13, 0);
                ui_element_set_scale((UIElement *) checkbox, 0.6f);
                ui_element_set_action((UIElement *) checkbox, checkbox_action);

                ui_set_checkbox_checked(checkbox, *setting->var);
                ui_element_add_child(card, (UIElement *) checkbox);
            }

            UILabel *name = ui_create_label(&screen.ctx);
            if (name) {
                ui_label_set_text(name, setting->label);
                ui_element_set_position((UIElement *) name, -list_width + 28, 0);
                ui_element_set_scale((UIElement *) name, 0.38f);

                ui_element_add_child(card, (UIElement *) name);
            }

            if (setting->additionalInfo) {
                UIButton *info = ui_create_button(&screen.ctx);
                if (info) {
                    // Store the text pointer in the user data
                    InfoButtonData *data = malloc(sizeof(*data));
                    data->text = setting->additionalInfo;
                    ui_element_set_userdata((UIElement *) info, data);

                    ui_element_set_position((UIElement *) info, list_width -+ 13, 0);
                    ui_element_set_scale((UIElement *) info, 0.5f);
                    ui_element_set_action((UIElement *) info, info_action);

                    ui_button_set_image(info, 90, 0);

                    ui_element_add_child(card, (UIElement *) info);
                }
            }

            ui_list_add(list, card);
        }
    }
}

void load_category(SettingPage page) {
    ui_list_reset(list);
    ui_label_set_text(category_name, category_names[page]);
    ui_run_func_on_tag(&screen, "category", set_button_style);

    int count = 0;
    for (int i = 0; i < ARRAY_LEN(settings); i++) {
        Setting *setting = &settings[i];

        if (setting->page == page) {
            create_setting(setting, count);
            count++;
        }
    }
}

static UIAction actions[] = {
    { "exit", exit_settings },
    {"category", action_category}
};

void settings_init() {
    ui_load_screen(&screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/settings.txt");
    ui_screen_open(&screen, ANIM_ZOOM);
    yes_exit = false;

    list = (UIList *) ui_get_element_by_tag(&screen, "list");
    category_name = (UILabel *) ui_get_element_by_tag(&screen, "listlabel");

    current_page = 0;

    load_category(current_page);
}

int settings_loop() {
    if (yes_exit) {
        cfg_save();
        
        ui_unload_screen(&screen);
        return true;
    }

    if (request_list_reload) {
        load_category(current_page);
        request_list_reload = false;
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