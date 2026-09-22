
#include "particles/particle_definitions.h"
#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "graphics.h"

void ui_particle_update_pos(UIParticle* e){
    if (!e) return;

    ParticleSystem *p = &(e->particle);

    p->emitterX = e->base.x;
    p->emitterY = e->base.y;
}

void ui_particle_emit(UIParticle* e, int emitCount){
    if (!e) return;

    spawnMultipleParticles(&(e->particle), emitCount);
}

static void ui_particle_update(UIElement* e, UIInput* touch, UITransform *transform) {
    UIParticle *particle = (UIParticle *) e;
    ParticleSystem *p = &(particle->particle);

    updateParticleSystem(p, delta);
}

static void ui_particle_draw(UIElement* e, UITransform *transform) {
    UIParticle *particle = (UIParticle *) e;
    
    change_blending(true);
    drawParticleSystem(&(particle->particle), 0, 0, 1);
    change_blending(false);
}

static void ui_particle_destroy(UIElement *e) {
    if (e) {
        UIParticle *particle = (UIParticle *) e;
        freeParticleData(&particle->particle.data);
        free(e);

        particle = NULL;
        e = NULL;
    }
}

void ui_init_particle_definition(UIParticle *e, const ParticleDefinition *def) {
    if (!e || !def) return;

    ParticleSystem *p = &(e->particle);
    initParticleSystem(p, def);
    p->affectedByMirror = false;
    p->stationary = true;
}

UIParticle *ui_create_particle(UIScreen *screen) {
    UIParticle *e = malloc(sizeof(UIParticle));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIParticle));

    e->base.type = UI_PARTICLE;
    e->base.enabled = true;

    e->base.update = ui_particle_update;
    e->base.draw = ui_particle_draw;
    e->base.destroy = ui_particle_destroy;
    
    ui_element_apply_default_properties(&e->base, screen);

    return e;
}

UIElement *ui_create_particle_from_props(UIScreen *screen, const UIPropertyList *props) {
    UIParticle *particle = ui_create_particle(screen);

    if (!particle) return NULL;
    
    ui_element_apply_properties(&particle->base, screen, props);

    float r = ui_prop_float(props, "r", -1.f);
    float g = ui_prop_float(props, "g", -1.f);
    float b = ui_prop_float(props, "b", -1.f);

    ParticleSystem *p = &(particle->particle);
    //Do not override colors if invalid
    if(r >= 0.f && g >= 0.f && b >= 0.f && r <= 1.f && g <= 1.f && b <= 1.f){
        p->cfg.startColorRed = r;
        p->cfg.startColorGreen = g;
        p->cfg.startColorBlue = b;
        p->cfg.finishColorRed = r;
        p->cfg.finishColorGreen = g;
        p->cfg.finishColorBlue = b;
    }

    const char *def_name = ui_prop_string(props, "particleDef", "");

    const ParticleDefinition *particleDef = NULL;

    for(int i = 0; i < PARTICLE_DEF_COUNT; i++){
        if(strcmp(particleDefNames[i].name, def_name) == 0){
            particleDef = particleDefNames[i].def;
        }
    }

    ui_init_particle_definition(particle, particleDef);

    p->emitterX = particle->base.x;
    p->emitterY = particle->base.y;
    p->scale = particle->base.scaleX;

    return &particle->base;
}