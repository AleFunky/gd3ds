#pragma once
#include "main.h"
#include <3ds.h>

typedef enum {
    PAGE_GRAPHICS,
    PAGE_INPUT,
    PAGE_MISC,
    PAGE_GAMEPLAY,
    PAGE_COSMETIC
} SettingPage;

typedef struct {
    const char *id;      
    const char *label;          // Setting name
    const char *additionalInfo; // Extra info popup message
       
    SettingPage page;

    bool defaultValue;
    bool *var;       // Pointer to the value that will contain the setting's value
    const char *key; // Config file key

    void (*onChanged)(bool);

    bool disabledForceValue; // Value that will be forced upon the condition failing
    bool (*condition)();
} Setting;

typedef struct {
    bool wideEnabled;
    bool stereoEnabled;
    bool particlesDisabled;
    bool glowEnabled;
    bool yJump;
    bool touchEffectEverywhere;
    bool enableDebugBindings;
    bool hitboxesEnabled;
    bool hitboxTrail;
    bool hitboxesOnDeath;
    bool showProgressBar;
    bool showProgressPercent;
    bool decimalPercent;
    bool ultraDecimalPercent;
    bool defaultMiniIcon;
    bool switchTrailColor;
    bool switchWaveTrailColor;
    bool quickRetry;
    bool solidWaveTrail;
    bool noPlayerTrail;
    bool noWaveTrailBehind;
    bool doNot;
    bool practiceMusicSync;
    bool autoCheckpoints;
    bool quickCheckpoints;
    bool skipHighObjWarning;
    bool skipVersionWarning;
    bool skipSongWarning;
} SettingState;

extern Setting settings[29];
extern SettingState settingsState;

void settings_init();
int settings_loop();