#include "particles.hpp"
#include <math.h>
#include "particle_definitions.hpp"
#include <stdlib.h>
#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <citro2d.h>
#include "state.hpp"
#include "math_helpers.hpp"
#include "utils/gfx.hpp"
#include "menus/settings_hub/settings.hpp"

static float randf() {
    return (float)rand() / RAND_MAX;
}

// Returns value in range [-1, 1]
static float randMinus11() {
    return randf() * 2.0f - 1.0f;
}

static void copyParticle(ParticleData* d, int dst, int src) {
    d->timeToLive[dst] = d->timeToLive[src];
    d->totalTimeToLive[dst] = d->totalTimeToLive[src];

    d->posx[dst] = d->posx[src];
    d->posy[dst] = d->posy[src];

    d->relx[dst] = d->relx[src];
    d->rely[dst] = d->rely[src];

    d->dirX[dst] = d->dirX[src];
    d->dirY[dst] = d->dirY[src];

    d->gravityX[dst] = d->gravityX[src];
    d->gravityY[dst] = d->gravityY[src];

    d->radialAccel[dst] = d->radialAccel[src];
    d->tangentialAccel[dst] = d->tangentialAccel[src];

    d->colorR[dst] = d->colorR[src];
    d->colorG[dst] = d->colorG[src];
    d->colorB[dst] = d->colorB[src];
    d->colorA[dst] = d->colorA[src];

    d->deltaColorR[dst] = d->deltaColorR[src];
    d->deltaColorG[dst] = d->deltaColorG[src];
    d->deltaColorB[dst] = d->deltaColorB[src];
    d->deltaColorA[dst] = d->deltaColorA[src];

    d->size[dst] = d->size[src];
    d->deltaSize[dst] = d->deltaSize[src];

    d->rotation[dst] = d->rotation[src];
    d->deltaRotation[dst] = d->deltaRotation[src];

    d->angle[dst] = d->angle[src];
    d->degreesPerSecond[dst] = d->degreesPerSecond[src];
    d->radius[dst] = d->radius[src];
    d->deltaRadius[dst] = d->deltaRadius[src];
}

void initParticle(ParticleSystem* ps, const ParticleDefinition* cfg, int i) {
    ParticleData* d = &ps->data;

    if (!ps->data.couldLoad) 
        return;

    // Lifetime
    float life = cfg->particleLifespan +
                 cfg->particleLifespanVariance * randMinus11();

    life = fabsf(life);

    d->timeToLive[i] = life;
    d->totalTimeToLive[i] = life;

    // Angle and speed
    float angle = C3D_AngleFromDegrees(cfg->angle +
                  cfg->angleVariance * randMinus11());
    
    // Position
    if (ps->posVarRotates) {
        float cosA = cosf(angle - C3D_AngleFromDegrees(90));
        float sinA = sinf(angle - C3D_AngleFromDegrees(90));

        float localX = cfg->sourcePositionVariancex * randMinus11();
        float localY = cfg->sourcePositionVariancey * randMinus11();

        float rotatedX = localX * cosA - localY * sinA;
        float rotatedY = localX * sinA + localY * cosA;
        
        d->relx[i] = rotatedX * ps->scale;
        d->rely[i] = rotatedY * ps->scale;

        d->posx[i] = d->relx[i] + ps->emitterX;
        d->posy[i] = d->rely[i] + ps->emitterY;
    } else {
        d->relx[i] = cfg->sourcePositionVariancex * randMinus11() * ps->scale;
        d->rely[i] = cfg->sourcePositionVariancey * randMinus11() * ps->scale;

        d->posx[i] = d->relx[i] + ps->emitterX;
        d->posy[i] = d->rely[i] + ps->emitterY;
    }

    
    float speed = (cfg->speed +
                  cfg->speedVariance * randMinus11()) * ps->scale;

    float speedY = ps->gravityFlipped ? -speed : speed;

    d->dirX[i] = cosf(angle) * speed;
    d->dirY[i] = sinf(angle) * speedY;

    d->gravityX[i] = ps->gravityX * ps->scale;
    d->gravityY[i] = (ps->gravityFlipped ? -ps->gravityY : ps->gravityY) * ps->scale;

    // Size
    float startSize = cfg->startParticleSize +
                      cfg->startParticleSizeVariance * randMinus11();
    startSize = fmaxf(0.0f, startSize * ps->scale);

    float endSize = cfg->finishParticleSize +
                    cfg->finishParticleSizeVariance * randMinus11();
    endSize = fmaxf(0.0f, endSize * ps->scale);

    d->size[i] = startSize;

    if (life > 0)
        d->deltaSize[i] = (endSize - startSize) / life;
    else
        d->deltaSize[i] = 0;

    // Color
    float sr = cfg->startColorRed +
               cfg->startColorVarianceRed * randMinus11();
    float sg = cfg->startColorGreen +
               cfg->startColorVarianceGreen * randMinus11();
    float sb = cfg->startColorBlue +
               cfg->startColorVarianceBlue * randMinus11();
    float sa = cfg->startColorAlpha +
               cfg->startColorVarianceAlpha * randMinus11();

    float er = cfg->finishColorRed +
               cfg->finishColorVarianceRed * randMinus11();
    float eg = cfg->finishColorGreen +
               cfg->finishColorVarianceGreen * randMinus11();
    float eb = cfg->finishColorBlue +
               cfg->finishColorVarianceBlue * randMinus11();
    float ea = cfg->finishColorAlpha +
               cfg->finishColorVarianceAlpha * randMinus11();

    d->colorR[i] = clampf(sr, 0, 1);
    d->colorG[i] = clampf(sg, 0, 1);
    d->colorB[i] = clampf(sb, 0, 1);
    d->colorA[i] = clampf(sa, 0, 1);

    if (life > 0) {
        d->deltaColorR[i] = (er - sr) / life;
        d->deltaColorG[i] = (eg - sg) / life;
        d->deltaColorB[i] = (eb - sb) / life;
        d->deltaColorA[i] = (ea - sa) / life;
    } else {
        d->deltaColorR[i] = 0;
        d->deltaColorG[i] = 0;
        d->deltaColorB[i] = 0;
        d->deltaColorA[i] = 0;
    }

    // Rotation
    float startRot = cfg->rotationStart +
                     cfg->rotationStartVariance * randMinus11();

    float endRot = cfg->rotationEnd +
                   cfg->rotationEndVariance * randMinus11();

    d->rotation[i] = startRot;

    if (life > 0)
        d->deltaRotation[i] = (endRot - startRot) / life;
    else
        d->deltaRotation[i] = 0;

    if (cfg->emitterType == 0) {

        // Radial / Tangential
        d->radialAccel[i] =
            (cfg->radialAcceleration +
            cfg->radialAccelVariance * randMinus11()) * ps->scale;

        d->tangentialAccel[i] =
            (cfg->tangentialAcceleration +
            cfg->tangentialAccelVariance * randMinus11()) * ps->scale;
            

    } else { // Radial emitter (Mode B)
        float startRadius = (cfg->maxRadius +
                            cfg->maxRadiusVariance * randMinus11()) * ps->scale;

        float endRadius = cfg->minRadius * ps->scale;

        d->radius[i] = startRadius;

        if (life > 0)
            d->deltaRadius[i] = (endRadius - startRadius) / life;
        else
            d->deltaRadius[i] = 0;

        float angle = C3D_AngleFromDegrees(cfg->angle +
                    cfg->angleVariance * randMinus11());

        d->angle[i] = angle;

        float degPerSec = cfg->rotatePerSecond +
                        cfg->rotatePerSecondVariance * randMinus11();

        d->degreesPerSecond[i] = C3D_AngleFromDegrees(degPerSec);
    }
}

void spawnMultipleParticles(ParticleSystem* ps, int emitCount) {
    if (settingsState.particlesDisabled) return;
    
    if (!ps->active || !ps->data.couldLoad)
        return;

    ParticleData* d = &ps->data;
    
    for (int i = 0; i < emitCount && d->count < d->capacity; i++) {
        int idx = d->count++;
        initParticle(ps, &ps->cfg, idx);
    }
}

void updateParticleSystem(ParticleSystem* ps, float dt) {
    if (settingsState.particlesDisabled) return;
    
    if (!ps->active || !ps->data.couldLoad)
        return;

    ParticleData* d = &ps->data;

    // Emission
    if (ps->emissionRate > 0.0f && ps->emitting) {
        float rate = 1.0f / ps->emissionRate;

        ps->emitCounter += dt;

        int emitCount = (int)(ps->emitCounter / rate);

        for (int i = 0; i < emitCount && d->count < d->capacity; i++) {
            int idx = d->count++;
            initParticle(ps, &ps->cfg, idx);
        }

        ps->emitCounter -= rate * emitCount;

        ps->elapsed += dt;
    }

    int count = d->count;

    // Time to live
    for (int i = 0; i < count; i++)
        d->timeToLive[i] -= dt;

    // Remove dead
    for (int i = 0; i < d->count; i++) {
        if (d->timeToLive[i] <= 0.0f) {
            int last = d->count - 1;
            copyParticle(d, i, last);
            d->count--;
            i--; // Recheck swapped
        }
    }

    count = d->count;

    if (ps->cfg.emitterType == 0) { // Gravity Mode (Mode A)
        for (int i = 0; i < count; i++) {
            float px = d->relx[i];
            float py = d->rely[i];

            float radialX = 0.0f;
            float radialY = 0.0f;

            if (px != 0.0f || py != 0.0f)
            {
                float len = sqrtf(px * px + py * py);
                radialX = px / len;
                radialY = py / len;
            }

            float tangentialX = -radialY;
            float tangentialY = radialX;

            radialX *= d->radialAccel[i];
            radialY *= d->radialAccel[i];

            tangentialX *= d->tangentialAccel[i];
            tangentialY *= d->tangentialAccel[i];

            float ax = d->gravityX[i] + radialX + tangentialX;
            float ay = d->gravityY[i] + radialY + tangentialY;

            d->dirX[i] += ax * dt;
            d->dirY[i] += ay * dt;

            d->posx[i] += d->dirX[i] * dt;
            d->posy[i] += d->dirY[i] * dt;

            d->relx[i] += d->dirX[i] * dt;
            d->rely[i] += d->dirY[i] * dt;
        }
    } else { // Radial mode (Mode B)
        // update angle
        for (int i = 0; i < count; i++) {
            d->angle[i] += d->degreesPerSecond[i] * dt;
        }

        // update radius
        for (int i = 0; i < count; i++) {
            d->radius[i] += d->deltaRadius[i] * dt;
        }

        // convert polar -> cartesian
        for (int i = 0; i < count; i++) {
            d->relx[i] = -cosf(d->angle[i]) * d->radius[i];
            d->posx[i] = ps->emitterX + d->relx[i];
        }

        for (int i = 0; i < count; i++) {
            d->rely[i] = -sinf(d->angle[i]) * d->radius[i];
            d->posy[i] = ps->emitterY + d->rely[i];
        }
    }

    // Color
    for (int i = 0; i < count; i++)
        d->colorR[i] += d->deltaColorR[i] * dt;

    for (int i = 0; i < count; i++)
        d->colorG[i] += d->deltaColorG[i] * dt;

    for (int i = 0; i < count; i++)
        d->colorB[i] += d->deltaColorB[i] * dt;

    for (int i = 0; i < count; i++)
        d->colorA[i] += d->deltaColorA[i] * dt;

    // Size
    for (int i = 0; i < count; i++) {
        d->size[i] += d->deltaSize[i] * dt;
        if (d->size[i] < 0) d->size[i] = 0;
    }

    // Rotation
    for (int i = 0; i < count; i++)
        d->rotation[i] += d->deltaRotation[i] * dt;
}

static float* allocArray(int count) {
    return (float*)malloc(sizeof(float) * count);
}

void initParticleData(ParticleData* d, int capacity) {
    d->count = 0;
    d->capacity = capacity;
    d->couldLoad = false;

    d->timeToLive = allocArray(capacity);
    if (!d->timeToLive) return;

    d->totalTimeToLive = allocArray(capacity);
    if (!d->totalTimeToLive) return;

    d->posx = allocArray(capacity);
    if (!d->posx) return;
    d->posy = allocArray(capacity);
    if (!d->posy) return;

    d->relx = allocArray(capacity);
    if (!d->relx) return;
    d->rely = allocArray(capacity);
    if (!d->rely) return;

    d->dirX = allocArray(capacity);
    if (!d->dirX) return;
    d->dirY = allocArray(capacity);
    if (!d->dirY) return;

    d->gravityX = allocArray(capacity);
    if (!d->gravityX) return;
    d->gravityY = allocArray(capacity);
    if (!d->gravityY) return;

    d->radialAccel = allocArray(capacity);
    if (!d->radialAccel) return;
    d->tangentialAccel = allocArray(capacity);
    if (!d->tangentialAccel) return;

    d->colorR = allocArray(capacity);
    if (!d->colorR) return;
    d->colorG = allocArray(capacity);
    if (!d->colorG) return;
    d->colorB = allocArray(capacity);
    if (!d->colorB) return;
    d->colorA = allocArray(capacity);
    if (!d->colorA) return;

    d->deltaColorR = allocArray(capacity);
    if (!d->deltaColorR) return;
    d->deltaColorG = allocArray(capacity);
    if (!d->deltaColorG) return;
    d->deltaColorB = allocArray(capacity);
    if (!d->deltaColorB) return;
    d->deltaColorA = allocArray(capacity);
    if (!d->deltaColorA) return;

    d->size = allocArray(capacity);
    if (!d->size) return;
    d->deltaSize = allocArray(capacity);
    if (!d->deltaSize) return;

    d->rotation = allocArray(capacity);
    if (!d->rotation) return;
    d->deltaRotation = allocArray(capacity);
    if (!d->deltaRotation) return;

    // Mode B
    d->angle = allocArray(capacity);
    if (!d->angle) return;
    d->degreesPerSecond = allocArray(capacity);
    if (!d->degreesPerSecond) return;
    d->radius = allocArray(capacity);
    if (!d->radius) return;
    d->deltaRadius = allocArray(capacity);
    if (!d->deltaRadius) return;

    d->couldLoad = true;
}

void initParticleSystem(ParticleSystem* ps, const ParticleDefinition* cfg) {
    // Allocate particles
    initParticleData(&ps->data, cfg->maxParticles);

    // Basic state
    ps->active = true;
    ps->emitting = false;
    ps->gravityFlipped = false;
    ps->elapsed = 0.0f;
    ps->duration = cfg->duration;
    ps->emitterX = cfg->sourcePositionx;
    ps->emitterY = cfg->sourcePositiony;
    ps->scale = 1.0f;
    ps->depth = 0.0f;
    ps->depthInwards = false;
    ps->posVarRotates = false;
    ps->affectedByMirror = true;

    ps->emitCounter = 0.0f;

    // Gravity
    ps->gravityX = cfg->gravityx;
    ps->gravityY = cfg->gravityy;

    ps->cfg = *cfg;
    if (cfg->particleLifespan <= 0) {
        ps->cfg.particleLifespanVariance /= 2;
        ps->cfg.particleLifespan = ps->cfg.particleLifespanVariance;
    }
    
    // Emission rate
    if (cfg->particleLifespan > 0)
        ps->emissionRate = cfg->maxParticles / cfg->particleLifespan;
    else 
        ps->emissionRate = 0;
}

void freeParticleData(ParticleData* d) {
    // Free deez nuts
    if (d->timeToLive) free(d->timeToLive);
    if (d->totalTimeToLive) free(d->totalTimeToLive);

    if (d->posx) free(d->posx);
    if (d->posy) free(d->posy);
    
    if (d->relx) free(d->relx);
    if (d->rely) free(d->rely);
    
    if (d->dirX) free(d->dirX);
    if (d->dirY) free(d->dirY);
    
    if (d->gravityX) free(d->gravityX);
    if (d->gravityY) free(d->gravityY);
    
    if (d->radialAccel) free(d->radialAccel);
    if (d->tangentialAccel) free(d->tangentialAccel);
    
    if (d->colorR) free(d->colorR);
    if (d->colorG) free(d->colorG);
    if (d->colorB) free(d->colorB);
    if (d->colorA) free(d->colorA);
    
    if (d->deltaColorR) free(d->deltaColorR);
    if (d->deltaColorG) free(d->deltaColorG);
    if (d->deltaColorB) free(d->deltaColorB);
    if (d->deltaColorA) free(d->deltaColorA);
    
    if (d->size) free(d->size);
    if (d->deltaSize) free(d->deltaSize);

    if (d->rotation) free(d->rotation);
    if (d->deltaRotation) free(d->deltaRotation);

    if (d->angle) free(d->angle);
    if (d->degreesPerSecond) free(d->degreesPerSecond);
    if (d->radius) free(d->radius);
    if (d->deltaRadius) free(d->deltaRadius);
}

void drawParticleSystem(ParticleSystem* ps, float xOffset, float yOffset, float opacity) {
    if (settingsState.particlesDisabled) return;
    
    C2D_Image img = C2D_SpriteSheetGetImage(particleSheet, ps->cfg.textureFileName);
    C3D_TexSetFilter(img.tex, GPU_LINEAR, GPU_LINEAR);
    
    ParticleData* d = &ps->data;
    int count = d->count;

    // Only worth checking per particle when there's actually 3D to show
    bool useDepth = ps->depth && isStereoActive();

    for (int i = 0; i < count; i++) {
        float x = d->posx[i];
        float y = d->posy[i];
        float size = d->size[i];

        float r = d->colorR[i];
        float g = d->colorG[i];
        float b = d->colorB[i];
        float a = d->colorA[i];

        u32 color = C2D_Color32f(r, g, b, a * opacity);

        if (!ps->relativeStationary) {
            // If stationary, dont convert to screen space
            if (!ps->stationary) {
                if (!ps->dontApplyCamX) x = ((x - state.cameraX));
                if (!ps->dontApplyCamY) y = GSP_SCREEN_WIDTH - ((y - state.cameraY));  
            } else {
                x = d->relx[i] + ps->emitterX;
                y = d->rely[i] + ps->emitterY;
            }

            if (ps->affectedByMirror) {
                // Apply mirror
                x = getMirrorX(x + xOffset, state.mirrorFactor);
            } else {
                x += xOffset;
            }
        } else {
            // Flip
            x += xOffset;
            y = (GSP_SCREEN_WIDTH - y);
        }

        // Drift out of the screen as the particle ages, or back into it if inverted
        if (useDepth && d->totalTimeToLive[i] > 0) {
            float life = d->timeToLive[i] / d->totalTimeToLive[i];
            x += getDepthShift(ps->depth * (ps->depthInwards ? life : 1.f - life));
        }

        C2D_ImageTint tint = { 0 };

        C2D_PlainImageTint(&tint, color, 1.f);

        C2D_DrawImageAtRotated(
            img, 
            x,
            y + yOffset,
            0.0f,
            d->rotation[i], 
            &tint,
            size / 32.f,
            size / 32.f
        );
    }
}