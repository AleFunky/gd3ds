#pragma once

#include "menus/core/ui_screen.h"
#include "particles/particle_definitions.h"

void ui_init_particle_definition(UIParticle *e, const ParticleDefinition *def);
void ui_particle_update_pos(UIParticle* e);
void ui_particle_emit(UIParticle* e, int emitCount);

UIParticle *ui_create_particle(UIScreen *screen);
UIElement *ui_create_particle_from_props(UIScreen *screen, const UIPropertyList *props);