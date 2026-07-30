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
#include "utils/json_config.h"

Config cfg;

void init_user_config(Config *cfg) {
    for (int i = 0; i < ARRAY_LEN(settings); i++) {
        config_init_bool(cfg,
            settings[i].key,
            settings[i].defaultValue
        );
    }
}

void load_user_config(Config *cfg) {
    for (int i = 0; i < ARRAY_LEN(settings); i++) {
        *settings[i].var = 
            config_get_bool(cfg,
                settings[i].key,
                settings[i].defaultValue
            );
    }
}

void save_user_config(Config *cfg) {
    for (int i = 0; i < ARRAY_LEN(settings); i++) {
        config_set_bool(
            cfg,
            settings[i].key,
            *settings[i].var
        );
    }
}

void init_values() {
    config_init_bool(&cfg, CONFIG_FLAGS "initialDisclaimerAccepted", false);
    config_init_bool(&cfg, CONFIG_FLAGS "sogged", false);
    
    config_init_int(&cfg, CONFIG_VALUES "playersDestroyed", 0);
    config_init_float(&cfg, CONFIG_VALUES "music_volume", 1);
    config_init_float(&cfg, CONFIG_VALUES "sound_volume", 1);

    init_user_config(&cfg);

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
    config_init_bool(&cfg, CONFIG_FILTERS_PATH "length", false);
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

    load_user_config(&cfg);
    
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

    uncompletedFilter = config_get_bool(&cfg, CONFIG_FILTERS_PATH "uncompleted", false);
    completedFilter   = config_get_bool(&cfg, CONFIG_FILTERS_PATH "completed", false);
    originalFilter    = config_get_bool(&cfg, CONFIG_FILTERS_PATH "original", false);
    unratedFilter     = config_get_bool(&cfg, CONFIG_FILTERS_PATH "unrated", false);
    ratedFilter       = config_get_bool(&cfg, CONFIG_FILTERS_PATH "rated", false);
    featuredFilter    = config_get_bool(&cfg, CONFIG_FILTERS_PATH "featured", false);
    songFilter        = config_get_bool(&cfg, CONFIG_FILTERS_PATH "song", false);
    lengthFilter        = config_get_bool(&cfg, CONFIG_FILTERS_PATH "length", false);
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

    save_user_config(&cfg);

    config_set_bool(&cfg, CONFIG_FILTERS_PATH "uncompleted", uncompletedFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "completed", completedFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "original", originalFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "unrated", unratedFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "rated", ratedFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "featured", featuredFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "length", lengthFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "song", songFilter);
    config_set_bool(&cfg, CONFIG_FILTERS_PATH "customSelected", customSelected);
    config_set_int(&cfg, CONFIG_FILTERS_PATH "normalId", normalSongId);
    config_set_string(&cfg, CONFIG_FILTERS_PATH "songId", songFilterId);

    config_save(&cfg);
}

void cfg_fini() {
    config_free(&cfg);
}