#include "config.hpp"

#include <3ds.h>
#include "main.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include "menus/icon_kit.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/creator_menu/soggy.hpp"
#include "menus/creator_menu/search_menu.hpp"
#include "menus/updater_settings.hpp"

#include "save/saving.hpp"
#include "utils/json_config.hpp"

#include "utils/server_utils.hpp"

namespace gd3ds
{

Config cfg;

void initUserConfig(Config *cfg) {
    for (size_t i = 0; i < ARRAY_LEN(settings); i++) {
        if (!settings[i].var) continue;

        configInitBool(cfg,
            settings[i].key,
            settings[i].defaultValue
        );
    }
}

void loadUserConfig(Config *cfg) {
    for (size_t i = 0; i < ARRAY_LEN(settings); i++) {
        Setting *setting = &settings[i];

        if (!setting->var) continue;

        // Ensure the default value if the condition is false
        if (setting->condition && !setting->condition()) {
            *setting->var = setting->disabledForceValue;
        } else {
            *setting->var = 
                configGetBool(cfg,
                    setting->key,
                    setting->defaultValue
                );
        }
    }
}

void saveUserConfig(Config *cfg) {
    for (size_t i = 0; i < ARRAY_LEN(settings); i++) {
        Setting *setting = &settings[i];

        if (!setting->var) continue;
        
        // Ensure the default value if the condition is false
        if (setting->condition && !setting->condition()) {
            configSetBool(
                cfg,
                setting->key,
                setting->disabledForceValue
            );
        } else {
            configSetBool(
                cfg,
                setting->key,
                *setting->var
            );
        }
    }
}

void initValues() {
    configInitBool(&cfg, CONFIG_FLAGS "sogged", false);
    configInitBool(&cfg, CONFIG_FLAGS "gdps", false);
    configInitBool(&cfg, CONFIG_FLAGS "nightlyUpdates", isNightly);
    configInitBool(&cfg, CONFIG_FLAGS "autoUpdate", false);
    configInitBool(&cfg, CONFIG_FLAGS "3dWarningPassed", false);
    
    configInitInt(&cfg, CONFIG_VALUES "playersDestroyed", 0);
    configInitFloat(&cfg, CONFIG_VALUES "music_volume", 1);
    configInitFloat(&cfg, CONFIG_VALUES "sound_volume", 1);
    configInitFloat(&cfg, CONFIG_COSMETIC_PATH "dimSecondaryScreen", 0);

    initUserConfig(&cfg);

    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", 1);
    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", 1);
    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", 1);
    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  1);
    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", 1);
    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "trail", 0);
    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   DEFAULT_P1);
    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   DEFAULT_P2);
    configInitInt(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", DEFAULT_GLOW);
    configInitBool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", false);

    configInitBool(&cfg, CONFIG_FILTERS_PATH "uncompleted", false);
    configInitBool(&cfg, CONFIG_FILTERS_PATH "completed", false);
    configInitBool(&cfg, CONFIG_FILTERS_PATH "original", false);
    configInitBool(&cfg, CONFIG_FILTERS_PATH "unrated",  false);
    configInitBool(&cfg, CONFIG_FILTERS_PATH "rated",  false);
    configInitBool(&cfg, CONFIG_FILTERS_PATH "featured", false);
    configInitBool(&cfg, CONFIG_FILTERS_PATH "super", false);
    configInitInt(&cfg, CONFIG_FILTERS_PATH "length", false);
    configInitBool(&cfg, CONFIG_FILTERS_PATH "song", false);
    configInitBool(&cfg, CONFIG_FILTERS_PATH "customSelected", false);
    configInitInt(&cfg, CONFIG_FILTERS_PATH "normalId", 0);
    configInitString(&cfg, CONFIG_FILTERS_PATH "songId", "");
}

void cfgInit() {
    // Make the directories
    mkdir(CONFIG_PARENT, 0777);
    mkdir(CONFIG_ROOT, 0777);
    mkdir(USER_LEVELS_DIR, 0777);
    mkdir(USER_SONGS_DIR, 0777);
    mkdir(USER_SONGS_DIR, 0777);
    mkdir(SAVED_LEVELS_DIR, 0777);

    configLoad(&cfg, CONFIG_FILE);

    initValues();

    doAutoUpdates = configGetBool(&cfg, CONFIG_FLAGS "autoUpdate", false);
    useNightlyBranch = configGetBool(&cfg, CONFIG_FLAGS "nightlyUpdates", false);

    gotSogged = configGetBool(&cfg, CONFIG_FLAGS "sogged", false);
    gdps = configGetBool(&cfg, CONFIG_FLAGS "gdps", false);
    strcpy(menuLoopPath, gdps ? "romfs:/songs/menuLoopGDPS.mp3" : "romfs:/songs/menuLoop.mp3");
    threeDWarningPassed = configGetBool(&cfg, CONFIG_FLAGS "3dWarningPassed", false);

    playersDestroyed = configGetInt(&cfg, CONFIG_VALUES "playersDestroyed", 0);
    musicVolume = configGetFloat(&cfg, CONFIG_VALUES "music_volume", 1);
    soundVolume = configGetFloat(&cfg, CONFIG_VALUES "sound_volume", 1);

    loadUserConfig(&cfg);
    dimSecondaryScreen = configGetFloat(&cfg, CONFIG_COSMETIC_PATH "dimSecondaryScreen", 0);
    
    selectedCube = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", 1);
    selectedShip = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", 1);
    selectedBall = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", 1);
    selectedUfo  = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  1);
    selectedWave = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", 1);
    selectedRobot = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "robot", 1);
    if (selectedRobot < 1 || selectedRobot >= ICON_COUNT_ROBOT) selectedRobot = 1;
    selectedSpider = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "spider", 1);
    if (selectedSpider < 1 || selectedSpider >= ICON_COUNT_SPIDER) selectedSpider = 1;
    selectedTrail = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "trail", 0);
    selectedP1   = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   DEFAULT_P1);
    selectedP2   = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   DEFAULT_P2);
    selectedGlow = configGetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", DEFAULT_GLOW);
    playerGlowEnabled = configGetBool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", false);

    filters.isNA = configGetBool(&cfg, CONFIG_FILTERS_PATH "na", false);
    filters.isAuto = configGetBool(&cfg, CONFIG_FILTERS_PATH "auto", false);
    filters.isDemon = configGetBool(&cfg, CONFIG_FILTERS_PATH "demon", false);
    filters.difficultyFilters = configGetInt(&cfg, CONFIG_FILTERS_PATH "difficulties", false);
    filters.uncompleted = configGetBool(&cfg, CONFIG_FILTERS_PATH "uncompleted", false);
    filters.completed   = configGetBool(&cfg, CONFIG_FILTERS_PATH "completed", false);
    filters.original    = configGetBool(&cfg, CONFIG_FILTERS_PATH "original", false);
    filters.noStar      = configGetBool(&cfg, CONFIG_FILTERS_PATH "unrated", false);
    filters.star        = configGetBool(&cfg, CONFIG_FILTERS_PATH "rated", false);
    filters.featured    = configGetBool(&cfg, CONFIG_FILTERS_PATH "featured", false);
    filters.super       = configGetBool(&cfg, CONFIG_FILTERS_PATH "super", false);
    filters.songFilter  = configGetBool(&cfg, CONFIG_FILTERS_PATH "song", false);
    filters.lengthFilters = configGetInt(&cfg, CONFIG_FILTERS_PATH "length", false);
    filters.customSong  = configGetBool(&cfg, CONFIG_FILTERS_PATH "customSelected", false);
    filters.mainSong    = configGetInt(&cfg, CONFIG_FILTERS_PATH "normalId", 0);
    strncpy(filters.customSongQuery, configGetString(&cfg, CONFIG_FILTERS_PATH "songId", ""), sizeof(filters.customSongQuery) - 1);

    configSave(&cfg);
}

void cfgSave() {
    configSetBool(&cfg, CONFIG_FLAGS "autoUpdate", doAutoUpdates);
    configSetBool(&cfg, CONFIG_FLAGS "nightlyUpdates", useNightlyBranch);
    configSetBool(&cfg, CONFIG_FLAGS "sogged", gotSogged);
    configSetBool(&cfg, CONFIG_FLAGS "gdps", gdps);
    configSetBool(&cfg, CONFIG_FLAGS "3dWarningPassed", threeDWarningPassed);

    configSetInt(&cfg, CONFIG_VALUES "playersDestroyed", playersDestroyed);
    configSetFloat(&cfg, CONFIG_VALUES "music_volume", musicVolume);
    configSetFloat(&cfg, CONFIG_VALUES "sound_volume", soundVolume);

    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "cube", selectedCube);
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ship", selectedShip);
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ball", selectedBall);
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "ufo",  selectedUfo );
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "wave", selectedWave);
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "robot", selectedRobot);
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "spider", selectedSpider);
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "trail", selectedTrail);
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "p1",   selectedP1  );
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "p2",   selectedP2  );
    configSetInt(&cfg, CONFIG_CUSTOMIZATION_PATH "glow", selectedGlow);
    configSetBool(&cfg, CONFIG_CUSTOMIZATION_PATH "playerGlowEnabled", playerGlowEnabled);

    saveUserConfig(&cfg);
    configSetFloat(&cfg, CONFIG_COSMETIC_PATH "dimSecondaryScreen", dimSecondaryScreen);

    configSetBool(&cfg, CONFIG_FILTERS_PATH "na", filters.isNA);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "auto", filters.isAuto);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "demon", filters.isDemon);
    configSetInt(&cfg, CONFIG_FILTERS_PATH "difficulties", filters.difficultyFilters);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "uncompleted", filters.uncompleted);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "completed", filters.completed);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "original", filters.original);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "unrated", filters.noStar);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "rated", filters.star);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "featured", filters.featured);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "super", filters.super);
    configSetInt(&cfg, CONFIG_FILTERS_PATH "length", filters.lengthFilters);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "song", filters.songFilter);
    configSetBool(&cfg, CONFIG_FILTERS_PATH "customSelected", filters.customSong);
    configSetInt(&cfg, CONFIG_FILTERS_PATH "normalId", filters.mainSong);
    configSetString(&cfg, CONFIG_FILTERS_PATH "songId", filters.customSongQuery);

    beginSaving(SAVE_CONFIG);
}

void cfgFini() {
    configFree(&cfg);
}

}