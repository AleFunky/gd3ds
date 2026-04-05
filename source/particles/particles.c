#include "particles.h"
#include <math.h>
#include "particle_definitions.h"
#include <stdlib.h>
#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <citro2d.h>
#include "state.h"
#include "math_helpers.h"
#include "utils/gfx.h"
#include "menus/settings.h"

static float randf() {
    return (float)rand() / RAND_MAX;
}

// Returns value in range [-1, 1]
static float rand_minus1_1() {
    return randf() * 2.0f - 1.0f;
}

static void copy_particle(ParticleData* d, int dst, int src) {
    d->timeToLive[dst] = d->timeToLive[src];
    d->totalTimeToLive[dst] = d->totalTimeToLive[src];

    d->posx[dst] = d->posx[src];
    d->posy[dst] = d->posy[src];

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

    // Lifetime
    float life = cfg->particleLifespan +
                 cfg->particleLifespanVariance * rand_minus1_1();

    life = fmaxf(0.0f, life);

    d->timeToLive[i] = life;
    d->totalTimeToLive[i] = life;

    // Angle and speed
    float angle = C3D_AngleFromDegrees(cfg->angle +
                  cfg->angleVariance * rand_minus1_1());
    
                  // Position
    if (ps->posVarRotates) {
        float cosA = cosf(angle - C3D_AngleFromDegrees(90));
        float sinA = sinf(angle - C3D_AngleFromDegrees(90));

        float local_x = cfg->sourcePositionVariancex * rand_minus1_1();
        float local_y = cfg->sourcePositionVariancey * rand_minus1_1();

        float rotated_x = local_x * cosA - local_y * sinA;
        float rotated_y = local_x * sinA + local_y * cosA;
        

        d->posx[i] = (ps->stationary ? 0 : ps->emitterX) + rotated_x * ps->scale;
        d->posy[i] = (ps->stationary ? 0 : ps->emitterY) + rotated_y * ps->scale;
    } else {
        
        d->posx[i] = (ps->stationary ? 0 : ps->emitterX) +
                    cfg->sourcePositionVariancex * rand_minus1_1() * ps->scale;

        d->posy[i] = (ps->stationary ? 0 : ps->emitterY) +
                    cfg->sourcePositionVariancey * rand_minus1_1() * ps->scale;
    }
    
    float speed = (cfg->speed +
                  cfg->speedVariance * rand_minus1_1()) * ps->scale;

    float speed_y = ps->gravityFlipped ? -speed : speed;

    d->dirX[i] = cosf(angle) * speed;
    d->dirY[i] = sinf(angle) * speed_y;

    d->gravityX[i] = ps->gravityX * ps->scale;
    d->gravityY[i] = (ps->gravityFlipped ? -ps->gravityY : ps->gravityY) * ps->scale;

    // Size
    float startSize = cfg->startParticleSize +
                      cfg->startParticleSizeVariance * rand_minus1_1();
    startSize = fmaxf(0.0f, startSize * ps->scale);

    float endSize = cfg->finishParticleSize +
                    cfg->finishParticleSizeVariance * rand_minus1_1();
    endSize = fmaxf(0.0f, endSize * ps->scale);

    d->size[i] = startSize;

    if (life > 0)
        d->deltaSize[i] = (endSize - startSize) / life;
    else
        d->deltaSize[i] = 0;

    // Color
    float sr = cfg->startColorRed +
               cfg->startColorVarianceRed * rand_minus1_1();
    float sg = cfg->startColorGreen +
               cfg->startColorVarianceGreen * rand_minus1_1();
    float sb = cfg->startColorBlue +
               cfg->startColorVarianceBlue * rand_minus1_1();
    float sa = cfg->startColorAlpha +
               cfg->startColorVarianceAlpha * rand_minus1_1();

    float er = cfg->finishColorRed +
               cfg->finishColorVarianceRed * rand_minus1_1();
    float eg = cfg->finishColorGreen +
               cfg->finishColorVarianceGreen * rand_minus1_1();
    float eb = cfg->finishColorBlue +
               cfg->finishColorVarianceBlue * rand_minus1_1();
    float ea = cfg->finishColorAlpha +
               cfg->finishColorVarianceAlpha * rand_minus1_1();

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
                     cfg->rotationStartVariance * rand_minus1_1();

    float endRot = cfg->rotationEnd +
                   cfg->rotationEndVariance * rand_minus1_1();

    d->rotation[i] = startRot;

    if (life > 0)
        d->deltaRotation[i] = (endRot - startRot) / life;
    else
        d->deltaRotation[i] = 0;

    if (cfg->emitterType == 0) {

        // Radial / Tangential
        d->radialAccel[i] =
            (cfg->radialAcceleration +
            cfg->radialAccelVariance * rand_minus1_1()) * ps->scale;

        d->tangentialAccel[i] =
            (cfg->tangentialAcceleration +
            cfg->tangentialAccelVariance * rand_minus1_1()) * ps->scale;
            

    } else { // Radial emitter (Mode B)
        float startRadius = (cfg->maxRadius +
                            cfg->maxRadiusVariance * rand_minus1_1()) * ps->scale;

        float endRadius = cfg->minRadius * ps->scale;

        d->radius[i] = startRadius;

        if (life > 0)
            d->deltaRadius[i] = (endRadius - startRadius) / life;
        else
            d->deltaRadius[i] = 0;

        float angle = C3D_AngleFromDegrees(cfg->angle +
                    cfg->angleVariance * rand_minus1_1());

        d->angle[i] = angle;

        float degPerSec = cfg->rotatePerSecond +
                        cfg->rotatePerSecondVariance * rand_minus1_1();

        d->degreesPerSecond[i] = C3D_AngleFromDegrees(degPerSec);
    }
}

void spawnMultipleParticles(ParticleSystem* ps, int emitCount) {
    if (particlesDisabled) return;
    
    if (!ps->active)
        return;

    ParticleData* d = &ps->data;
    
    for (int i = 0; i < emitCount && d->count < d->capacity; i++) {
        int idx = d->count++;
        initParticle(ps, &ps->cfg, idx);
    }
}

void updateParticleSystem(ParticleSystem* ps, float dt) {
    if (particlesDisabled) return;
    
    if (!ps->active)
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
            copy_particle(d, i, last);
            d->count--;
            i--; // Recheck swapped
        }
    }

    count = d->count;

    if (ps->cfg.emitterType == 0) { // Gravity Mode (Mode A)
        for (int i = 0; i < count; i++) {
            float px = d->posx[i] - ps->emitterX;
            float py = d->posy[i] - ps->emitterY;

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
            d->posx[i] = ps->emitterX + -cosf(d->angle[i]) * d->radius[i];
        }

        for (int i = 0; i < count; i++) {
            d->posy[i] = ps->emitterY + -sinf(d->angle[i]) * d->radius[i];
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

static float* alloc_array(int count) {
    return (float*)malloc(sizeof(float) * count);
}

void initParticleData(ParticleData* d, int capacity) {
    d->count = 0;
    d->capacity = capacity;

    d->timeToLive = alloc_array(capacity);
    d->totalTimeToLive = alloc_array(capacity);

    d->posx = alloc_array(capacity);
    d->posy = alloc_array(capacity);

    d->dirX = alloc_array(capacity);
    d->dirY = alloc_array(capacity);

    d->gravityX = alloc_array(capacity);
    d->gravityY = alloc_array(capacity);

    d->radialAccel = alloc_array(capacity);
    d->tangentialAccel = alloc_array(capacity);

    d->colorR = alloc_array(capacity);
    d->colorG = alloc_array(capacity);
    d->colorB = alloc_array(capacity);
    d->colorA = alloc_array(capacity);

    d->deltaColorR = alloc_array(capacity);
    d->deltaColorG = alloc_array(capacity);
    d->deltaColorB = alloc_array(capacity);
    d->deltaColorA = alloc_array(capacity);

    d->size = alloc_array(capacity);
    d->deltaSize = alloc_array(capacity);

    d->rotation = alloc_array(capacity);
    d->deltaRotation = alloc_array(capacity);

    // Mode B
    d->angle = alloc_array(capacity);
    d->degreesPerSecond = alloc_array(capacity);
    d->radius = alloc_array(capacity);
    d->deltaRadius = alloc_array(capacity);
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

    ps->emitCounter = 0.0f;

    // Emission rate
    if (cfg->particleLifespan > 0)
        ps->emissionRate = cfg->maxParticles / cfg->particleLifespan;
    else 
        ps->emissionRate = 0;
        
    // Gravity
    ps->gravityX = cfg->gravityx;
    ps->gravityY = cfg->gravityy;

    ps->cfg = *cfg;
    if (cfg->particleLifespan <= 0) {
        ps->cfg.particleLifespanVariance /= 2;
        ps->cfg.particleLifespan = ps->cfg.particleLifespanVariance;
    }
}

void freeParticleData(ParticleData* d) {
    // Free deez nuts
    free(d->timeToLive);
    free(d->totalTimeToLive);

    free(d->posx);
    free(d->posy);

    free(d->dirX);
    free(d->dirY);

    free(d->gravityX);
    free(d->gravityY);

    free(d->radialAccel);
    free(d->tangentialAccel);

    free(d->colorR);
    free(d->colorG);
    free(d->colorB);
    free(d->colorA);

    free(d->deltaColorR);
    free(d->deltaColorG);
    free(d->deltaColorB);
    free(d->deltaColorA);

    free(d->size);
    free(d->deltaSize);

    free(d->rotation);
    free(d->deltaRotation);

    free(d->angle);
    free(d->degreesPerSecond);
    free(d->radius);
    free(d->deltaRadius);
}

void drawParticleSystem(ParticleSystem* ps, float x_offset, float y_offset, float opacity) {
    if (particlesDisabled) return;
    
    ParticleData* d = &ps->data;
    int count = d->count;

    for (int i = 0; i < count; i++) {
        float x = d->posx[i];
        float y = d->posy[i];
        float size = d->size[i];

        float r = d->colorR[i];
        float g = d->colorG[i];
        float b = d->colorB[i];
        float a = get_opacity(d->colorA[i]);

        u32 color = C2D_Color32f(r, g, b, a * opacity);

        if (!ps->relativeStationary) {
            // If stationary, dont convert to screen space
            if (!ps->stationary) {
                x = ((x - state.camera_x));
                y = GSP_SCREEN_WIDTH - ((y - state.camera_y));  
            } else {
                x += ps->emitterX;
                y += ps->emitterY;
            }

            // Apply mirror
            x = get_mirror_x(x, state.mirror_factor);
        } else {
            // Flip
            y = GSP_SCREEN_WIDTH - y;
        }

        // Draw centered square
        float half = size * 0.5f;

        C2D_DrawRectSolid(
            x - half + x_offset,
            y - half + y_offset,
            0.0f,
            size,
            size,
            color
        );
    }
}