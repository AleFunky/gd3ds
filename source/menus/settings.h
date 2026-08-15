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
    const char *label;
    const char *additionalInfo;
       
    SettingPage page;

    bool defaultValue;
    bool *var;
    const char *key;

    void (*onChanged)(bool);
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
} SettingState;

extern Setting settings[25];
extern SettingState settingsState;

void settings_init();
int settings_loop();