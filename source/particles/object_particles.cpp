#include "object_particles.hpp"
#include <math.h>
#include "level_loading.hpp"
#include "triggers.hpp"
#include "particles/particles.hpp"
#include "state.hpp"
#include "main.hpp"
#include "player/collision.hpp"
#include "math_helpers.hpp"

namespace gd3ds
{

ObjectParticles objectParticle[MAX_OBJECT_PS];

ParticleSystem brickDestroyParticles;

// probably inaccurate, especially the alpha, but it's osmething lmao
static const ParticleDefinition collectableParticles = {
    .angleVariance = 180.0f,
    .duration = -1.0f,
    .maxParticles = 36.0f,
    .particleLifespan = 0.6f,
    .particleLifespanVariance = 0.15f,
    .sourcePositionVariancex = 15.0f,
    .sourcePositionVariancey = 10.0f,
    .speed = 5.0f,
    .speedVariance = 5.0f,
    .startColorAlpha = 1.0f,
    .startColorBlue = 1.0f,
    .startColorGreen = 1.0f,
    .startColorRed = 1.0f,
    .startParticleSize = 1.5f,
    .startParticleSizeVariance = 1.1f,
    .textureFileName = TEXTURE_SQUARE_PNG,
};

void initOpSystem() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        objectParticle[i].id = -1;
        objectParticle[i].occupied = false;
        objectParticle[i].colorApplied = false;
    }
}

bool isPsAlreadyLoaded(int id) {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (objectParticle[i].occupied && objectParticle[i].id == id) {
            return true;
        }
    }
    return false;
}

// How far the particles an object gives off drift towards you before they die
static float getObjectParticleDepth(int id) {
    switch (id) {
        case YELLOW_PAD:
        case BLUE_PAD:
        case PINK_PAD:
        case CUBE_PORTAL:
        case SHIP_PORTAL:
        case BALL_PORTAL:
        case UFO_PORTAL:
        case WAVE_PORTAL:
        case BLUE_GRAVITY_PORTAL:
        case YELLOW_GRAVITY_PORTAL:
        case ORANGE_MIRROR_PORTAL:
        case BLUE_MIRROR_PORTAL:
        case BIG_PORTAL:
        case MINI_PORTAL:
        case SLOW_SPEED_PORTAL:
        case NORMAL_SPEED_PORTAL:
        case FAST_SPEED_PORTAL:
        case FASTER_SPEED_PORTAL:
        case FASTEST_SPEED_PORTAL:
        case DUAL_PORTAL:
        case DIVORCE_PORTAL:
        case ROBOT_PORTAL:
        case SPIDER_PORTAL:
        case BLUE_TP_PORTAL:
        case ORANGE_TP_PORTAL:
            return 0.5f;
        // Orbs and coins hang in mid air, so theirs carry a bit further
        case YELLOW_ORB:
        case BLUE_ORB:
        case PINK_ORB:
        case GREEN_ORB:
        case RED_ORB:
        case BLACK_ORB:
        case DASH_ORB:
        case GRAVITY_DASH_ORB:
        case TOGGLE_ORB:
        case SECRET_COIN:
            return 0.7f;
    }
    return 0.f;
}

int loadObjectParticles(int id, const ParticleDefinition *cfg, bool stationary) {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (!objectParticle[i].occupied) {
            objectParticle[i].occupied = true;
            objectParticle[i].id = id;
            objectParticle[i].rotationBaseSet = false;
            objectParticle[i].ps.stationary = stationary;
            initParticleSystem(&objectParticle[i].ps, cfg);

            // After the init, it resets the depth back to flat
            objectParticle[i].ps.depth = getObjectParticleDepth(objects.id[id]);

            // The small portals and orbs spiral their particles in, so the drift follows
            objectParticle[i].ps.depthInwards = (cfg == &portalEffect01 || cfg == &ringEffect);

            objectParticle[i].ps.emitting = true;
            return i;
        }
    }
    return -1;
}

void freeObjectParticles() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (objectParticle[i].occupied) {
            freeParticleData(&objectParticle[i].ps.data);
            objectParticle[i].occupied = false;
        }
    }
}

static void removeOffscreenObjectParticles() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (objectParticle[i].occupied) {
            float x = objectParticle[i].ps.emitterX;
            float y = objectParticle[i].ps.emitterY;

            if (!objectParticle[i].ps.stationary) {
                x = ((x - state.cameraX));
                y = SCREEN_HEIGHT - ((y - state.cameraY));  
            }

            if (x < -60 || x >= (SCREEN_WIDTH / SCALE) + 60 || y < -60 || y >= (SCREEN_HEIGHT / SCALE) + 60) {
                objectParticle[i].occupied = false;
                objectParticle[i].colorApplied = false;
                freeParticleData(&objectParticle[i].ps.data);
            }
        }
    }
}

static void rotateObjectParticles(ParticleSystem *particleSystem, float deltaDegrees, float emitterX, float emitterY) {
    ParticleData *data = &particleSystem->data;
    if (!data->couldLoad || data->count <= 0) return;
    float rotationRadians = -deltaDegrees * (float)(M_PI / 180.0);
    if (particleSystem->cfg.emitterType != 0) {
        for (int particleIndex = 0; particleIndex < data->count; particleIndex++)
            data->angle[particleIndex] += rotationRadians;
        return;
    }
    float cosAngle = cosf(rotationRadians);
    float sinAngle = sinf(rotationRadians);
    for (int particleIndex = 0; particleIndex < data->count; particleIndex++) {
        float offsetX = data->posx[particleIndex] - particleSystem->emitterX;
        float offsetY = data->posy[particleIndex] - particleSystem->emitterY;
        data->posx[particleIndex] = emitterX + offsetX * cosAngle - offsetY * sinAngle;
        data->posy[particleIndex] = emitterY + offsetX * sinAngle + offsetY * cosAngle;
        float relativeX = data->relx[particleIndex];
        float relativeY = data->rely[particleIndex];
        data->relx[particleIndex] = relativeX * cosAngle - relativeY * sinAngle;
        data->rely[particleIndex] = relativeX * sinAngle + relativeY * cosAngle;
        float velocityX = data->dirX[particleIndex];
        float velocityY = data->dirY[particleIndex];
        data->dirX[particleIndex] = velocityX * cosAngle - velocityY * sinAngle;
        data->dirY[particleIndex] = velocityX * sinAngle + velocityY * cosAngle;
    }
}

void updateObjectParticles(float dt) {
    removeOffscreenObjectParticles();
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (objectParticle[i].occupied) {
            
            int obj = objectParticle[i].id;
            float emitterX = objects.x[obj];
            float emitterY = objects.y[obj];
            if (!objectParticle[i].rotationBaseSet) {
                objectParticle[i].rotationBase = objects.rotation[obj];
                objectParticle[i].angleBase = objectParticle[i].ps.cfg.angle;
                objectParticle[i].lastRotation = objects.rotation[obj];
                objectParticle[i].rotationBaseSet = true;
            } else if (objects.rotation[obj] != objectParticle[i].lastRotation) {
                float rotationDelta = objects.rotation[obj] - objectParticle[i].lastRotation;
                objectParticle[i].lastRotation = objects.rotation[obj];
                rotateObjectParticles(&objectParticle[i].ps, rotationDelta, emitterX, emitterY);
                objectParticle[i].ps.cfg.angle = objectParticle[i].angleBase - (objects.rotation[obj] - objectParticle[i].rotationBase);
            }
            objectParticle[i].ps.emitterX = emitterX;
            objectParticle[i].ps.emitterY = emitterY;
            bool toggled = (objects.flags[obj] & FLAG_TOGGLED) != 0;
            if (isCollectableObject(objects.id[obj])) {
                objectParticle[i].ps.emitting = !toggled;
                if (toggled) objectParticle[i].ps.data.count = 0;
                objectParticle[i].ps.scale = fmaxf(fabsf(objects.scaleX[obj]), fabsf(objects.scaleY[obj]));
                objectParticle[i].ps.cfg.startColorRed = objects.mainColor[obj].r / 255.f;
                objectParticle[i].ps.cfg.startColorGreen = objects.mainColor[obj].g / 255.f;
                objectParticle[i].ps.cfg.startColorBlue = objects.mainColor[obj].b / 255.f;
            } else if (toggled) objectParticle[i].ps.emitting = false;

            updateParticleSystem(&objectParticle[i].ps, dt);
        }
    }
}


void drawObjectParticles() {
    for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
        if (objectParticle[i].occupied) {
            int obj = objectParticle[i].id;
            float fadeX = 0;
            float fadeY = 0;

            float calcX = ((objects.x[obj] - state.cameraX));
            
            float fadeScale = 1.f;
            getFadeVars(obj, calcX, &fadeX, &fadeY, &fadeScale);

            float fadeVal = objEdgeFade(calcX, SCREEN_WIDTH / SCALE);

            // Calculate stationary fade positions
            fadeX += getSpecialFadingVars(obj, fadeVal);

            drawParticleSystem(&objectParticle[i].ps, fadeX, fadeY, (fadeVal / 255.f) * objects.alphaTriggerOpacity[obj]);
        }
    }
}

void setParticleColor(ParticleDefinition *cfg, float red, float green, float blue) {
    cfg->startColorRed = red;
    cfg->startColorGreen = green;
    cfg->startColorBlue = blue;

    cfg->finishColorRed = red;
    cfg->finishColorGreen = green;
    cfg->finishColorBlue = blue;
}

void spawnObjectParticles(int obj) {
    int index;
    switch (objects.id[obj]) {
        case YELLOW_ORB:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 1, 0);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLUE_ORB:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 1);
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case PINK_ORB:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0, 1);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case GREEN_ORB:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 0);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case RED_ORB:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0.39216f, 0);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLACK_ORB:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 1, 1);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case DASH_ORB:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 0);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case GRAVITY_DASH_ORB:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0, 0);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case TOGGLE_ORB:
            index = -1;
            for (size_t i = 0; i < MAX_OBJECT_PS; i++) {
                if (objectParticle[i].occupied && objectParticle[i].id == obj) { index = (int)i; break; }
            }

            if (index < 0) {
                index = loadObjectParticles(obj, &ringEffect, false);
                if (index >= 0) {
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }

            if (index >= 0) {
                if (!objectParticle[index].colorApplied) {
                    Color base = channels[getColChannelIndex(objects.detailColChannel[obj])].nonPulseColor;
                    setParticleColor(&objectParticle[index].ps.cfg, base.r / 255.f, base.g / 255.f, base.b / 255.f);
                    objectParticle[index].colorApplied = true;
                }
            }
            break;
        case YELLOW_PAD:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &bumpEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 1, 0);

                    objectParticle[index].ps.cfg.angle = 180.f - (adjustAngleY(objects.rotation[obj], objects.flippedV[obj]) + 90.f);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                    objectParticle[index].ps.posVarRotates = true;
                }
            }
            break;
        case BLUE_PAD:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &bumpEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 1);

                    objectParticle[index].ps.cfg.angle = 180.f - (adjustAngleY(objects.rotation[obj], objects.flippedV[obj]) + 90.f);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                    objectParticle[index].ps.posVarRotates = true;
                }
            }
            break;
        case PINK_PAD:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &bumpEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0, 1);

                    objectParticle[index].ps.cfg.angle = 180.f - (adjustAngleY(objects.rotation[obj], objects.flippedV[obj]) + 90.f);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                    objectParticle[index].ps.posVarRotates = true;
                }
            }
            break;
        case RED_PAD:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &bumpEffect, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0.19608f, 0.19608f);

                    objectParticle[index].ps.cfg.angle = 180.f - (adjustAngleY(objects.rotation[obj], objects.flippedV[obj]) + 90.f);

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                    objectParticle[index].ps.posVarRotates = true;
                }
            }
            break;
        case YELLOW_GRAVITY_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 1, 0);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLUE_GRAVITY_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 1);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case CUBE_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 0.25f);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case SHIP_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0, 1);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BALL_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0.25f, 0.25f);
                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case UFO_PORTAL:
        case ORANGE_MIRROR_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0.75f, 0);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case ROBOT_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 1, 1);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case SPIDER_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0.752941f, 0.133333f, 0.972549f);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BLUE_TP_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0.22f, 0.78f, 1.0f);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case ORANGE_TP_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1.0f, 91.0f/255.0f, 0.0f);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case WAVE_PORTAL:
        case BLUE_MIRROR_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect01, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 1);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case BIG_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect08, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 0.25f);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case MINI_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect08, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0.5f, 1);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case DUAL_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect08, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 0.75f, 0);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case DIVORCE_PORTAL:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &portalEffect08, false);
                if (index >= 0) {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 0.69f, 1);

                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));

                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case SLOW_SPEED_PORTAL:
            if (!isPsAlreadyLoaded(obj))
            {
                index = loadObjectParticles(obj, &boost01Effect, false);
                if (index >= 0)
                {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, 1, 0);
                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case NORMAL_SPEED_PORTAL:
            if (!isPsAlreadyLoaded(obj))
            {
                index = loadObjectParticles(obj, &boost02Effect, false);
                if (index >= 0)
                {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 0.75f, 1);
                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case FAST_SPEED_PORTAL:
            if (!isPsAlreadyLoaded(obj))
            {
                index = loadObjectParticles(obj, &boost03Effect, false);
                if (index >= 0)
                {
                    setParticleColor(&objectParticle[index].ps.cfg, 0, 1, 0);
                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case FASTER_SPEED_PORTAL:
            if (!isPsAlreadyLoaded(obj))
            {
                index = loadObjectParticles(obj, &boost04Effect, false);
                if (index >= 0)
                {
                    setParticleColor(&objectParticle[index].ps.cfg, 0.9f, 0.25f, 1);
                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case FASTEST_SPEED_PORTAL:
            if (!isPsAlreadyLoaded(obj))
            {
                index = loadObjectParticles(obj, &boost04Effect, false);
                if (index >= 0)
                {
                    setParticleColor(&objectParticle[index].ps.cfg, 1.f, 0.f, 0);
                    objectParticle[index].ps.cfg.angle = -(adjustAngleY(objects.rotation[obj], objects.flippedH[obj]));
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                }
            }
            break;
        case SECRET_COIN:
            if (!isPsAlreadyLoaded(obj))
            {
                if (isCoinCollected(obj)) break;
                
                index = loadObjectParticles(obj, &coinEffect, false);
                if (index >= 0)
                {
                    setParticleColor(&objectParticle[index].ps.cfg, 1, state.customLevel ? 1.f : 0.75f, state.customLevel ? 1.f : 0);
                    // object_particle[index].ps.cfg.angle = -(adjust_angle_y(objects.rotation[obj], objects.flippedH[obj]));
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj] - 15;
                }
            }
            break;
        case KEY_OBJ:
        case HEART_COLLECTABLE:
        case POTION_COLLECTABLE:
        case SKULL_COLLECTABLE:
            if (!isPsAlreadyLoaded(obj)) {
                index = loadObjectParticles(obj, &collectableParticles, false);
                if (index >= 0) {
                    objectParticle[index].ps.emitterX = objects.x[obj];
                    objectParticle[index].ps.emitterY = objects.y[obj];
                    objectParticle[index].ps.posVarRotates = true;
                }
            }
            break;
    }
}

}