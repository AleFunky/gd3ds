#pragma once
#include "menus/core/ui_screen.hpp"
#include <3ds.h>

namespace gd3ds
{

enum class SettingPage {
    PageGraphics,
    PageInput,
    PageMisc,
    PageGameplay,
    PageCosmetic
};

struct Setting {
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

    int buttonIcon;
    int buttonSheet;
    const char *buttonActions;
};

struct SettingState {
    bool wideEnabled;
    bool stereoEnabled;
    bool particlesDisabled;
    bool glowEnabled;
    bool yJump;
    bool touchEffectEverywhere;
    bool gameplayTapEffect;
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
    bool menuMusic;
    bool fastMenu;
    bool autoCheckpoints;
    bool quickCheckpoints;
    bool hidePracticeButtons;
    bool skipHighObjWarning;
    bool skipVersionWarning;
    bool skipSongWarning;
    bool vsync;
    bool disableGravityEffect;
    bool showFpsCounter;
    bool goldenBest;
    bool swapGameplayScreens;
};

extern Setting settings[39];
extern SettingState settingsState;

extern const UIScreenDefPair settingsDef;

bool conditionSupports3D();
void turnOffSetting(const char *id, bool *var);

}