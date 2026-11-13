#pragma once

#include "menus/core/ui_screen.hpp"
#include "particles/particle_definitions.hpp"

void uiInitParticleDefinition(UIParticle *e, const ParticleDefinition *def);
void uiParticleUpdatePos(UIParticle* e);
void uiParticleEmit(UIParticle* e, int emitCount);

UIParticle *uiCreateParticle(UIScreen *screen);
UIElement *uiCreateParticleFromProps(UIScreen *screen, const UIPropertyList *props);