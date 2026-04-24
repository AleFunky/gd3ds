#pragma once
#include <3ds.h>

typedef struct {
    const char *chk_name;
    bool *var;
} Setting;

extern bool particlesDisabled;
extern bool wideEnabled;
extern bool glowEnabled; 
extern bool yJump;
extern bool touchEffectEverywhere;
extern bool enableDebugBindings;
extern bool hitboxesEnabled;
extern bool hitboxTrail;
extern bool hitboxesOnDeath;

void settings_init();
int settings_loop();