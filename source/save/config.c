#include "config.h"

#include <3ds.h>
#include "main.h"
#include "graphics.h"

#include <sys/stat.h>
#include <sys/types.h>

#include "utils/gfx.h"
#include "menus/icon_kit.h"
#include "menus/settings.h"
#include "menus/search_filters.h"
#include "menus/first_boot_disclaimer.h"
#include "menus/soggy.h"

#include "save/saving.h"

Config cfg;

void init_values() {
    config_init_bool(&cfg, CONFIG_FLAGS "initialDisclaimerAccepted", false);
    config_init_bool(&cfg, CONFIG_FLAGS "sogged", false);
    
    config_init_int(&cfg, CONFIG_VALUES "playersDestroyed", 0);
    config_init_float(&cfg, CONFIG_VALUES "music_volume", 1);
    config_init_float(&cfg, CONFIG_VALUES "sound_volume", 1);

    config_init_bool(&cfg, CONFIG_GRAPHICS_PATH "particlesDisabled", false);
    config_init_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", false);
    config_init_bool(&cfg, CONFIG_GRAPHICS_PATH "glowEnabled", true);

    config_init_bool(&cfg, CONFIG_INPUT_PATH "yButton", false);
    config_init_bool(&cfg, CONFIG_INPUT_PATH "touchEffectEverywhere", false);
    config_init_bool(&cfg, CONFIG_INPUT_PATH "enableDebugBindings", false);

    config_init_bool(&cfg, CONFIG_MISC_PATH "hitboxesEnabled", false);
    config_init_bool(&cfg, CONFIG_MISC_PATH "hitboxTrail", false);
    config_init_bool(&cfg, CONFIG_MISC_PATH "hitboxesOnDeath", false);
    config_init_bool(&cfg, CONFIG_MISC_PATH "doNot", false);
    config_init_bool(&cfg, CONFIG_MISC_PATH "practiceMusicSync", false);

    config_init_bool(&cfg, CONFIG_GAMEPLAY_PATH "showProgressBar", false);
    config_init_bool(&cfg, CONFIG_GAMEPLAY_PATH "showProgressPercent", false);
    config_init_bool(&cfg, CONFIG_GAMEPLAY_PATH "decimalPercent", false);
    config_init_bool(&cfg, CONFIG_GAMEPLAY_PATH "ultraDecimalPercent", false);
    config_init_bool(&cfg, CONFIG_GAMEPLAY_PATH "quickRetry", false);

    config_init_bool(&cfg, CONFIG_COSMETIC_PATH "switchTrailColor", false);
    config_init_bool(&cfg, CONFIG_COSMETIC_PATH "switchWaveTrailColor", false);
    config_init_bool(&cfg, CONFIG_COSMETIC_PATH "solidWaveTrail", false);
    config_init_bool(&cfg, CONFIG_COSMETIC_PATH "noPlayerTrail", false);
    config_init_bool(&cfg, CONFIG_COSMETIC_PATH "noWaveTrailBehind", false);

    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", 1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", 1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", 1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", 1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "trail", 0);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   DEFAULT_P1);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   DEFAULT_P2);
    config_init_int(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", DEFAULT_GLOW);
    config_init_bool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", false);

    config_init_bool(&cfg, CONFIG_FILTERS_PATH "uncompleted", false);
    config_init_bool(&cfg, CONFIG_FILTERS_PATH "completed", false);
    config_init_bool(&cfg, CONFIG_FILTERS_PATH "original", false);
    config_init_bool(&cfg, CONFIG_FILTERS_PATH "unrated",  false);
    config_init_bool(&cfg, CONFIG_FILTERS_PATH "rated",  false);
    config_init_bool(&cfg, CONFIG_FILTERS_PATH "featured", false);
    config_init_bool(&cfg, CONFIG_FILTERS_PATH "song", false);
    config_init_bool(&cfg, CONFIG_FILTERS_PATH "customSelected", false);
    config_init_int(&cfg, CONFIG_FILTERS_PATH "normalId", 0);
    config_init_string(&cfg, CONFIG_FILTERS_PATH "songId", "");
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
    gotSogged = config_get_bool(&cfg, CONFIG_FLAGS "sogged", false);

    players_destroyed = config_get_int(&cfg, CONFIG_VALUES "playersDestroyed", 0);
    music_volume = config_get_float(&cfg, CONFIG_VALUES "music_volume", 1);
    sound_volume = config_get_float(&cfg, CONFIG_VALUES "sound_volume", 1);

    particlesDisabled = config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "particlesDisabled", false);
    set_wide(config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", false));
    glowEnabled = config_get_bool(&cfg, CONFIG_GRAPHICS_PATH "glowEnabled", true);

    hitboxesEnabled = config_get_bool(&cfg, CONFIG_MISC_PATH "hitboxesEnabled", false);
    hitboxTrail =     config_get_bool(&cfg, CONFIG_MISC_PATH "hitboxTrail", false);
    hitboxesOnDeath = config_get_bool(&cfg, CONFIG_MISC_PATH "hitboxesOnDeath", false);
    doNot = config_get_bool(&cfg, CONFIG_MISC_PATH "doNot", false);
    practiceMusicSync = config_get_bool(&cfg, CONFIG_MISC_PATH "practiceMusicSync", false);

    showProgressBar =     config_get_bool(&cfg, CONFIG_GAMEPLAY_PATH "showProgressBar", false);
    showProgressPercent = config_get_bool(&cfg, CONFIG_GAMEPLAY_PATH "showProgressPercent", false);
    decimalPercent =      config_get_bool(&cfg, CONFIG_GAMEPLAY_PATH "decimalPercent", false);
    ultraDecimalPercent = config_get_bool(&cfg, CONFIG_GAMEPLAY_PATH "ultraDecimalPercent", false);
    quickRetry = config_get_bool(&cfg, CONFIG_GAMEPLAY_PATH "quickRetry", false);

    switchTrailColor =     config_get_bool(&cfg, CONFIG_COSMETIC_PATH "switchTrailColor", false);
    switchWaveTrailColor = config_get_bool(&cfg, CONFIG_COSMETIC_PATH "switchWaveTrailColor", false);
    solidWaveTrail = config_get_bool(&cfg, CONFIG_COSMETIC_PATH "solidWaveTrail", false);
    noPlayerTrail = config_get_bool(&cfg, CONFIG_COSMETIC_PATH "noPlayerTrail", false);
    noWaveTrailBehind = config_get_bool(&cfg, CONFIG_COSMETIC_PATH "noWaveTrailBehind", false);

    selected_cube = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", 1);
    selected_ship = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", 1);
    selected_ball = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", 1);
    selected_ufo  = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  1);
    selected_wave = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", 1);
    selected_trail = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "trail", 0);
    selected_p1   = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   DEFAULT_P1);
    selected_p2   = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   DEFAULT_P2);
    selected_glow = config_get_int(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", DEFAULT_GLOW);
    player_glow_enabled = config_get_bool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", false);

    yJump =                 config_get_bool(&cfg, CONFIG_INPUT_PATH "yButton", false);
    touchEffectEverywhere = config_get_bool(&cfg, CONFIG_INPUT_PATH "touchEffectEverywhere", false);
    enableDebugBindings =   config_get_bool(&cfg, CONFIG_INPUT_PATH "enableDebugBindings", false);

    uncompletedFilter = config_get_bool(&cfg, CONFIG_FILTERS_PATH "uncompleted", false);
    completedFilter   = config_get_bool(&cfg, CONFIG_FILTERS_PATH "completed", false);
    originalFilter    = config_get_bool(&cfg, CONFIG_FILTERS_PATH "original", false);
    unratedFilter     = config_get_bool(&cfg, CONFIG_FILTERS_PATH "unrated", false);
    ratedFilter       = config_get_bool(&cfg, CONFIG_FILTERS_PATH "rated", false);
    featuredFilter    = config_get_bool(&cfg, CONFIG_FILTERS_PATH "featured", false);
    songFilter        = config_get_bool(&cfg, CONFIG_FILTERS_PATH "song", false);
    customSelected    = config_get_bool(&cfg, CONFIG_FILTERS_PATH "customSelected", false);
    normalSongId      = config_get_int(&cfg, CONFIG_FILTERS_PATH "normalId", 0);
    strncpy(songFilterId, config_get_string(&cfg, CONFIG_FILTERS_PATH "songId", ""), sizeof(songFilterId) - 1);

    config_save(&cfg);
}

void cfg_save() {
    config_set_bool(&cfg, CONFIG_FLAGS "initialDisclaimerAccepted", initialDisclaimerAccepted);
    config_set_bool(&cfg, CONFIG_FLAGS "sogged", gotSogged);

    config_set_int(&cfg, CONFIG_VALUES "playersDestroyed", players_destroyed);
    config_set_float(&cfg, CONFIG_VALUES "music_volume", music_volume);
    config_set_float(&cfg, CONFIG_VALUES "sound_volume", sound_volume);

    config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "particlesDisabled", particlesDisabled);
    config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "wideEnabled", wideEnabled);
    config_set_bool(&cfg, CONFIG_GRAPHICS_PATH "glowEnabled", glowEnabled);

    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", selected_cube);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", selected_ship);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", selected_ball);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  selected_ufo );
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", selected_wave);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "trail", selected_trail);
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   selected_p1  );
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   selected_p2  );
    config_set_int(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", selected_glow);
    config_set_bool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", player_glow_enabled);

    config_set_bool(&cfg, CONFIG_INPUT_PATH "yButton", yJump);
    config_set_bool(&cfg, CONFIG_INPUT_PATH "touchEffectEverywhere", touchEffectEverywhere);
    config_set_bool(&cfg, CONFIG_INPUT_PATH "enableDebugBindings", enableDebugBindings);

    config_set_bool(&cfg, CONFIG_MISC_PATH "hitboxesEnabled", hitboxesEnabled);
    config_set_bool(&cfg, CONFIG_MISC_PATH "hitboxTrail", hitboxTrail);
    config_set_bool(&cfg, CONFIG_MISC_PATH "hitboxesOnDeath", hitboxesOnDeath);
    config_set_bool(&cfg, CONFIG_MISC_PATH "doNot", doNot);
    config_set_bool(&cfg, CONFIG_MISC_PATH "practiceMusicSync", practiceMusicSync);

    config_set_bool(&cfg, CONFIG_GAMEPLAY_PATH "showProgressBar", showProgressBar);
    config_set_bool(&cfg, CONFIG_GAMEPLAY_PATH "showProgressPercent", showProgressPercent);
    config_set_bool(&cfg, CONFIG_GAMEPLAY_PATH "decimalPercent", decimalPercent);
    config_set_bool(&cfg, CONFIG_GAMEPLAY_PATH "ultraDecimalPercent", ultraDecimalPercent);
    config_set_bool(&cfg, CONFIG_GAMEPLAY_PATH "quickRetry", quickRetry);
    
    config_set_bool(&cfg, CONFIG_COSMETIC_PATH "switchTrailColor", switchTrailColor);
    config_set_bool(&cfg, CONFIG_COSMETIC_PATH "switchWaveTrailColor", switchWaveTrailColor);
    config_set_bool(&cfg, CONFIG_COSMETIC_PATH "solidWaveTrail", solidWaveTrail);
    config_set_bool(&cfg, CONFIG_COSMETIC_PATH "noPlayerTrail", noPlayerTrail);
    config_set_bool(&cfg, CONFIG_COSMETIC_PATH "noWaveTrailBehind", noWaveTrailBehind);

    config_set_bool(&cfg, CONFIG_FILTERS_PATH "uncompleted", uncompletedFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "completed", completedFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "original", originalFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "unrated", unratedFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "rated", ratedFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "featured", featuredFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "song", songFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "customSelected", customSelected);
    config_set_int(&cfg, CONFIG_FILTERS_PATH "normalId", normalSongId);
    config_set_string(&cfg, CONFIG_FILTERS_PATH "songId", songFilterId);

    config_save(&cfg);
}

void cfg_fini() {
    config_free(&cfg);
}