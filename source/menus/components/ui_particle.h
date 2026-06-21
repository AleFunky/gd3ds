#pragma once
#include "ui_element.h"
#include "particles/particle_definitions.h"

void ui_particle_update_pos(UIElement* e);
void ui_particle_emit(UIElement* e, int emitCount);
UIElement ui_create_particle(int x, int y, float scale, float r, float g, float b, const ParticleDefinition* particle, char (*tag)[TAG_LENGTH]);