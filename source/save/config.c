#include "config.h"

#include <3ds.h>
#include "main.h"
#include "graphics.h"

#include <sys/stat.h>
#include <sys/types.h>

#include "utils/gfx.h"
#include "menus/icon_kit.h"
#include "menus/settings.h"
#include "menus/first_boot_disclaimer.h"

Config cfg;

void init_values() {
    config_init_bool(&cfg, CONFIG_FLAGS "initialDisclaimerAccepted", false);

    config_init_bool(&cfg, CONFIG_GRAPHICS_PATH "particlesDisabled", false);
    config_init_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", false);
    config_init_bool(&cfg, CONFIG_GRAPHICS_PATH "glowEnabled", true);

    config_init_bool(&cfg, CONFIG_INPUT_PATH "yButton", false);
    config_init_bool(&cfg, CONFIG_INPUT_PATH "touchEffectEverywhere", false);

    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", 1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", 1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", 1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", 1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   DEFAULT_P1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   DEFAULT_P2);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", DEFAULT_GLOW);
    config_init_bool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", false);
}

void cfg_init() {
    // Make the directories
    mkdir(CONFIG_PARENT, 0777);
    mkdir(CONFIG_ROOT, 0777);
    mkdir(USER_LEVELS_DIR, 0777);
    mkdir(USER_SONGS_DIR, 0777);

    config_load(&cfg, CONFIG_FILE);

    init_values();

    initialDisclaimerAccepted = config_get_bool(&cfg, CONFIG_FLAGS "initialDisclaimerAccepted", false);

    particlesDisabled = config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "particlesDisabled", false);
    set_wide(config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", false));
    glowEnabled = config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "glowEnabled", true);

    selected_cube = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", 1);
    selected_ship = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", 1);
    selected_ball = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", 1);
    selected_ufo  = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  1);
    selected_wave = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", 1);
    selected_p1   = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   DEFAULT_P1);
    selected_p2   = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   DEFAULT_P2);
    selected_glow = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", DEFAULT_GLOW);
    player_glow_enabled = config_get_bool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", false);

    yJump = config_get_bool(&cfg, CONFIG_INPUT_PATH "yButton", false);
    touchEffectEverywhere = config_get_bool(&cfg, CONFIG_INPUT_PATH "touchEffectEverywhere", false);

    config_save(&cfg);
}

void cfg_save() {
    config_set_bool(&cfg, CONFIG_FLAGS "initialDisclaimerAccepted", initialDisclaimerAccepted);

    config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "particlesDisabled", particlesDisabled);
    config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", wideEnabled);
    config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "glowEnabled", glowEnabled);

    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", selected_cube);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", selected_ship);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", selected_ball);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  selected_ufo );
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", selected_wave);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   selected_p1  );
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   selected_p2  );
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", selected_glow);
    config_set_bool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", player_glow_enabled);

    config_set_bool(&cfg, CONFIG_INPUT_PATH "yButton", yJump);
    config_set_bool(&cfg, CONFIG_INPUT_PATH "touchEffectEverywhere", touchEffectEverywhere);

    config_save(&cfg);
}

void cfg_fini() {
    config_free(&cfg);
}