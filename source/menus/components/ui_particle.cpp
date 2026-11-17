
#include "main.hpp"
#include "particles/particle_definitions.hpp"
#include <citro2d.h>
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "graphics.hpp"

namespace gd3ds
{

void uiParticleUpdatePos(UIParticle* e){
    if (!e) return;

    ParticleSystem *p = &(e->particle);

    p->emitterX = e->base.x;
    p->emitterY = e->base.y;
}

void uiParticleEmit(UIParticle* e, int emitCount){
    if (!e) return;

    spawnMultipleParticles(&(e->particle), emitCount);
}

static void uiParticleUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    UIParticle *particle = (UIParticle *) e;
    ParticleSystem *p = &(particle->particle);

    updateParticleSystem(p, delta);
}

static void uiParticleDraw(UIElement* e, UITransform *transform) {
    UIParticle *particle = (UIParticle *) e;
    
    changeBlending(true);
    drawParticleSystem(&(particle->particle), 0, 0, 1);
    changeBlending(false);
}

static void uiParticleDestroy(UIElement *e) {
    if (e) {
        UIParticle *particle = (UIParticle *) e;
        freeParticleData(&particle->particle.data);
        delete (UIParticle *)e;

        particle = NULL;
        e = NULL;
    }
}

void uiInitParticleDefinition(UIParticle *e, const ParticleDefinition *def) {
    if (!e || !def) return;

    ParticleSystem *p = &(e->particle);
    initParticleSystem(p, def);
    p->affectedByMirror = false;
    p->stationary = true;
}

UIParticle *uiCreateParticle(UIScreen *screen) {
    UIParticle *e = new UIParticle{};

    if (!e) return NULL;

    e->base.type = UIElementType::UiParticle;
    e->base.enabled = true;

    e->base.update = uiParticleUpdate;
    e->base.draw = uiParticleDraw;
    e->base.destroy = uiParticleDestroy;
    
    uiElementApplyDefaultProperties(&e->base, screen);

    return e;
}

UIElement *uiCreateParticleFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIParticle *particle = uiCreateParticle(screen);

    if (!particle) return NULL;
    
    uiElementApplyProperties(&particle->base, screen, props);

    float r = uiPropFloat(props, "r", -1.f);
    float g = uiPropFloat(props, "g", -1.f);
    float b = uiPropFloat(props, "b", -1.f);

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

    const char *defName = uiPropString(props, "particleDef", "");

    const ParticleDefinition *particleDef = NULL;

    for(int i = 0; i < PARTICLE_DEF_COUNT; i++){
        if(strcmp(particleDefNames[i].name, defName) == 0){
            particleDef = particleDefNames[i].def;
        }
    }

    uiInitParticleDefinition(particle, particleDef);

    p->emitterX = particle->base.x;
    p->emitterY = particle->base.y;
    p->scale = particle->base.scaleX;

    return &particle->base;
}

}