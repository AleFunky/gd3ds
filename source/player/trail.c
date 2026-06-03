// Adapted from OpenGD

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "math.h"
#include "trail.h"

#include "main.h"

#include "state.h"

#include "mp3_player.h"
#include "math_helpers.h"
#include "utils/gfx.h"

#include "menus/settings.h"

const MotionTrailConfig trail_properties[TRAIL_COUNT] = {
    {. fade = 0.3f, .width = 10.f, .always_on = false, .colored = true,  .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 10.f, .always_on = false, .colored = true,  .stationary = false, .blending = true},
    {. fade = 0.4f, .width = 10.f, .always_on = false, .colored = true,  .stationary = false, .blending = true},
    {. fade = 0.5f, .width = 10.f, .always_on = true,  .colored = true,  .stationary = false, .blending = true},
    {. fade = 0.8f, .width = 3.f,  .always_on = true,  .colored = true,  .stationary = true, .blending = true },
    {. fade = 0.3f, .width = 10.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = false},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = false},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = true},
    {. fade = 0.3f, .width = 15.f, .always_on = false, .colored = false, .stationary = false, .blending = false},
};  

void MotionTrail_UpdateStationaryUVs(MotionTrail* trail, float offset);

// Adds "thickness" to a line strip by generating a triangle strip
void ccVertexLineToPolygon(const Vec2D* points, float stroke, Vec2D* outVerts, int offset, int count) {
    if (count < 2) return;

    float halfStroke = stroke * 0.5f;

    for (int i = offset; i < count; ++i) {
        Vec2D p = points[i];
        Vec2D dir;

        // Get direction
        if (i == 0) {
            dir = sub_vec(points[i + 1], p);
        } else if (i == count - 1) {
            dir = sub_vec(p, points[i - 1]);
        } else {
            dir = sub_vec(points[i + 1], points[i - 1]);
        }

        // Normalize direction
        dir = normalize(dir);

        // Perpendicular vector
        Vec2D perp = perpendicular(dir);

        // Offset left and right
        outVerts[i * 2]     = add_vec(p, multiply_vec(perp, halfStroke));
        outVerts[i * 2 + 1] = sub_vec(p, multiply_vec(perp, halfStroke));
    }
}
// Adds "vertical thickness" to a line strip by generating a triangle strip
void ccVertexLineToPolygonWave(const Vec2D* points, float stroke, Vec2D* outVerts, int offset, int count) {
    if (count < 2) return;

    float halfStroke = stroke * 0.5f;

    for (int i = offset; i < count; ++i) {
        Vec2D p = points[i];

        Vec2D perp;

        if (i == 0) {
            Vec2D dir = sub_vec(points[1], p);

            dir = normalize(dir);
            perp = perpendicular(dir);

            outVerts[i * 2]     = add_vec(p, multiply_vec(perp, halfStroke));
            outVerts[i * 2 + 1] = sub_vec(p, multiply_vec(perp, halfStroke));
        } else if (i == count - 1) {
            Vec2D dir = sub_vec(p, points[i - 1]);

            dir = normalize(dir);
            perp = perpendicular(dir);

            outVerts[i * 2]     = add_vec(p, multiply_vec(perp, halfStroke));
            outVerts[i * 2 + 1] = sub_vec(p, multiply_vec(perp, halfStroke));
        } else {
            // Previous segment direction
            Vec2D dirA = sub_vec(p, points[i - 1]); 

            // Next segment direction
            Vec2D dirB = sub_vec(points[i + 1], p);

            // Normalize
            dirA = normalize(dirA);
            dirB = normalize(dirB);

            // Normals
            Vec2D nA = perpendicular(dirA);
            Vec2D nB = perpendicular(dirB);

            // Average normals
            perp.x = nA.x + nB.x;
            perp.y = nA.y + nB.y;

            float perpLen = len_vec(perp);

            // Nearly opposite directions
            if (perpLen < 0.0001f) {
                perp = nB;
                perpLen = 1.0f;
            }

            perp.x /= perpLen;
            perp.y /= perpLen;

            float dot = dot_vec(dirA, dirB);

            // Prevent division explosions
            float denom = sqrtf(fmaxf((1.0f + dot) * 0.5f, 0.2f));

            float miterScale = 1.0f / denom;

            // Clamp sharp spikes
            if (miterScale > 2.0f)
                miterScale = 2.0f;

            float finalStroke = halfStroke * miterScale;

            outVerts[i * 2]     = add_vec(p, multiply_vec(perp, finalStroke));
            outVerts[i * 2 + 1] = sub_vec(p, multiply_vec(perp, finalStroke));
        }
    }
}

void MotionTrail_Clear(MotionTrail *trail) {
    trail->nuPoints = 0;
    trail->previousNuPoints = 0;
}

void MotionTrail_Init(MotionTrail* trail, int player, float fade, bool always_on, float stroke, bool waveTrail, bool blending, bool stationary, Color color, C2D_Image tex) {
    memset(trail, 0, sizeof(MotionTrail));
    trail->image = tex;  
    trail->maxPoints = MAX_TRAIL_POINTS;
    trail->fadeDelta = 1.0f / fade;
    trail->minSeg = 9;  // Compare squared distance
    trail->width = stroke;
    trail->color = color;
    trail->waveTrail = waveTrail;
    trail->nuPoints = 0;
    trail->previousNuPoints = 0;
    trail->blending = blending;
    trail->opacity = 1.f;
    trail->alwaysOn = always_on;
    trail->uvOffset = 0;
    trail->stationary = stationary;
    trail->player = player;
    if (!waveTrail) trail->appendNewPoints = true;
}

void MotionTrail_ResumeStroke(MotionTrail* trail) {
    if (!trail->appendNewPoints && trail->wasStopped) {
        float dx = trail->positionR.x - trail->lastStopPosition.x;
        float dy = trail->positionR.y - trail->lastStopPosition.y;

        if (square_distance(0, 0, dx, dy) > (TRAIL_CLEAR_DISTANCE * TRAIL_CLEAR_DISTANCE)) {
            trail->nuPoints = 0;
            trail->previousNuPoints = 0;
        }
        trail->wasStopped = false;
    }
    trail->appendNewPoints = true;
}

void MotionTrail_StopStroke(MotionTrail* trail) {
    if (trail->appendNewPoints) {
        trail->lastStopPosition = trail->positionR;
        trail->wasStopped = true;
        trail->appendNewPoints = false;
    }
}

void MotionTrail_CopyTrail(MotionTrail *dst, MotionTrail *src) {
    memcpy(&dst->pointState, &src->pointState, sizeof(src->pointState));
    memcpy(&dst->pointVertexes, &src->pointVertexes, sizeof(src->pointVertexes));
    memcpy(&dst->vertices, &src->vertices, sizeof(src->vertices));
    memcpy(&dst->centerVertices, &src->centerVertices, sizeof(src->centerVertices));
    dst->actualNuPoints = src->actualNuPoints;
    dst->previousNuPoints = src->previousNuPoints;
    dst->offscreenCount = src->offscreenCount;
}

void MotionTrail_Update(MotionTrail* trail, float delta) {
    if (trail->waveTrail) return;
    if (!trail->startingPositionInitialized) return;

    if (trail->alwaysOn) {
        Player *player = (trail->player ? &state.player2 : &state.player);

        if (state.mirroring || (player->gamemode == GAMEMODE_DART && noWaveTrailBehind)) {
            MotionTrail_StopStroke(trail);
        } else {
            MotionTrail_ResumeStroke(trail);
        }
    }

    delta *= trail->fadeDelta;

    unsigned int newIdx, newIdx2, i, i2;
    unsigned int mov = 0;

    // Fade old points
    for (i = 0; i < trail->nuPoints; i++) {
        trail->pointState[i] -= delta;
        if (trail->pointState[i] <= 0) {
            mov++;
            if (trail->stationary && i < trail->nuPoints - 1) {         
                trail->uvOffset += len_vec(
                    sub_vec(
                        trail->pointVertexes[i],
                        trail->pointVertexes[i + 1]
                    )
                );
            }
        } else {
            newIdx = i - mov;
            if (mov > 0) {
                trail->pointState[newIdx] = trail->pointState[i];
                trail->pointVertexes[newIdx] = trail->pointVertexes[i];

                i2 = i * 2;
                newIdx2 = newIdx * 2;
                trail->vertices[newIdx2] = trail->vertices[i2];
                trail->vertices[newIdx2 + 1] = trail->vertices[i2 + 1];
                trail->opacities[newIdx2] = trail->opacities[i2];
                trail->opacities[newIdx2 + 1] = trail->opacities[i2 + 1];
            }
            
            newIdx2 = newIdx * 2;
            u8 op = (u8)(trail->pointState[newIdx] * 255.0f);
            trail->opacities[newIdx2] = op;
            trail->opacities[newIdx2 + 1] = op;
        }
    }

    trail->nuPoints -= mov;

    // Append new point
    bool append = true;
    if (trail->nuPoints >= trail->maxPoints) {
        append = false;
    } else if (trail->nuPoints > 0) {
        bool a1 = square_dist_vec(&trail->pointVertexes[trail->nuPoints - 1], &trail->positionR) < trail->minSeg;
        bool a2 = (trail->nuPoints == 1) ? false : square_dist_vec(&trail->pointVertexes[trail->nuPoints - 2], &trail->positionR) < (trail->minSeg * 2.0f);
        if (a1 || a2) append = false;
    }

    if (append && trail->appendNewPoints) {
        unsigned int idx = trail->nuPoints;

        trail->pointVertexes[idx] = trail->positionR;
        trail->pointState[idx] = 1.0f;
        int offset = idx * 2;
        trail->opacities[offset] = 255;
        trail->opacities[offset + 1] = 255;

        trail->nuPoints++;
        
        if (trail->nuPoints > 1) {
            ccVertexLineToPolygon(trail->pointVertexes, trail->stroke, trail->vertices, 0, trail->nuPoints);
        }

    }

    if (!append || !trail->appendNewPoints) {
        if (trail->nuPoints > 1) {
            ccVertexLineToPolygon(trail->pointVertexes, trail->stroke, trail->vertices, 0, trail->nuPoints);
        }
    }

    // Update tex coords
    if (trail->nuPoints && trail->previousNuPoints != trail->nuPoints) {
        if (trail->stationary) {
            MotionTrail_UpdateStationaryUVs(trail, 0);
        } else {
            float texDelta = 1.0f / trail->nuPoints;
            for (i = 0; i < trail->nuPoints; i++) {
                trail->texCoords[i * 2].u = 0;
                trail->texCoords[i * 2].v = texDelta * i;
                trail->texCoords[i * 2 + 1].u = 1;
                trail->texCoords[i * 2 + 1].v = texDelta * i;
            }
        }
        trail->previousNuPoints = trail->nuPoints;
    } else if (trail->stationary) {
        MotionTrail_UpdateStationaryUVs(trail, trail->uvOffset);
    }
}

void MotionTrail_UpdateStationaryUVs(MotionTrail* trail, float offset) {
    float distance = 0.0f;

    for (int i = 0; i < trail->nuPoints; i++) {
        if (i > 0) {
            Vec2D d = sub_vec(
                trail->pointVertexes[i],
                trail->pointVertexes[i - 1]
            );

            distance += len_vec(d);
        }

        float v = reflect((trail->uvOffset + distance) / STATIONARY_TRAIL_CHUNK_SIZE, 0.f, 1.f);

        trail->texCoords[i * 2].u = 0.0f;
        trail->texCoords[i * 2].v = v;

        trail->texCoords[i * 2 + 1].u = 1.0f;
        trail->texCoords[i * 2 + 1].v = v;
    }
}

void MotionTrail_UpdateWaveTrail(MotionTrail* trail, float delta) {
    if (!trail->waveTrail) return;
    if (!trail->startingPositionInitialized) return;

    unsigned int mov = 0;
    unsigned int startIdx = 0;
    trail->offscreenCount = 0;
    
    // Get offscreen points
    for (unsigned int i = 0; i < trail->actualNuPoints; i++) {
        float x = trail->pointVertexes[i].x;
        float calc_x = (x - state.camera_x) + 6 * state.mirror_mult;  

        if (calc_x < 0) trail->offscreenCount++;
    }

    // Remove the first point if two or more points are offscreen
    if (trail->offscreenCount >= 2 && trail->nuPoints > 1) {
        startIdx = 1;
        mov = 1;
    }

    for (unsigned int i = startIdx; i < trail->nuPoints; ++i) {
        unsigned int newIdx = i - mov;

        if (mov > 0) {
            trail->pointState[newIdx] = trail->pointState[i];
            trail->pointVertexes[newIdx] = trail->pointVertexes[i];

            unsigned int i2 = i * 2;
            unsigned int newIdx2 = newIdx * 2;
            trail->vertices[newIdx2] = trail->vertices[i2];
            trail->vertices[newIdx2 + 1] = trail->vertices[i2 + 1];
        }
    }

    trail->nuPoints -= mov;

    trail->actualNuPoints = trail->nuPoints + 1;

    if (trail->actualNuPoints > 0) {
        trail->pointVertexes[trail->nuPoints] = trail->positionR;
    }

    if (trail->actualNuPoints > 1) {
        ccVertexLineToPolygonWave(trail->pointVertexes, trail->stroke, trail->vertices, 0, trail->actualNuPoints);
        ccVertexLineToPolygonWave(trail->pointVertexes, trail->stroke * 0.4f, trail->centerVertices, 0, trail->actualNuPoints);
    }
}

void MotionTrail_AddWavePoint(MotionTrail* trail) {
    if (!trail->waveTrail) return;
    if (trail->actualNuPoints >= trail->maxPoints) return;

    unsigned int idx = trail->nuPoints;

    trail->pointVertexes[idx] = trail->positionR;
    trail->startingPositionInitialized = true;
    trail->pointState[idx] = 1.0f;

    trail->nuPoints++;

    if (trail->nuPoints > 1) {
        ccVertexLineToPolygonWave(trail->pointVertexes, trail->stroke, trail->vertices, 0, trail->actualNuPoints);
        ccVertexLineToPolygonWave(trail->pointVertexes, trail->stroke * 0.4f, trail->centerVertices, 0, trail->actualNuPoints);
    }

    trail->previousNuPoints = trail->nuPoints;
}

void MotionTrail_Draw(MotionTrail* trail) {
    change_blending(trail->blending);
    
    C2D_Image image = trail->image;

    int count = trail->nuPoints * 2;

    Color color = trail->color;

    bool rotated = Tex3DS_SubTextureRotated(trail->image.subtex);

    for (int i = 0; i < count - 2; i++) {
        int i0 = i;
        int i1 = i + 1;
        int i2 = i + 2;

        Vec2D p0 = trail->vertices[i0];
        Vec2D p1 = trail->vertices[i1];
        Vec2D p2 = trail->vertices[i2];

        float x0 = get_mirror_x((p0.x - state.camera_x), state.mirror_factor);
        float y0 = SCREEN_HEIGHT - ((p0.y - state.camera_y));

        float x1 = get_mirror_x((p1.x - state.camera_x), state.mirror_factor);
        float y1 = SCREEN_HEIGHT - ((p1.y - state.camera_y));

        float x2 = get_mirror_x((p2.x - state.camera_x), state.mirror_factor);
        float y2 = SCREEN_HEIGHT - ((p2.y - state.camera_y));

        Tex2F t0 = trail->texCoords[i0];
        Tex2F t1 = trail->texCoords[i1];
        Tex2F t2 = trail->texCoords[i2];
        
        int opacity1 = trail->opacities[i0] * trail->opacity;
        int opacity2 = trail->opacities[i1] * trail->opacity;
        int opacity3 = trail->opacities[i2] * trail->opacity;

        const Tex3DS_SubTexture *subtex = trail->image.subtex;

        float u0;
        float v0;
        float u1;
        float v1;
        float u2;
        float v2;

        if (rotated) {
            u0 = map_range(t0.u, 0, 1, subtex->top, subtex->bottom);
            v0 = map_range(t0.v, 0, 1, subtex->left, subtex->right);
            u1 = map_range(t1.u, 0, 1, subtex->top, subtex->bottom);
            v1 = map_range(t1.v, 0, 1, subtex->left, subtex->right);
            u2 = map_range(t2.u, 0, 1, subtex->top, subtex->bottom);
            v2 = map_range(t2.v, 0, 1, subtex->left, subtex->right);
        } else {
            u0 = map_range(t0.u, 0, 1, subtex->left, subtex->right);
            v0 = map_range(t0.v, 0, 1, subtex->top, subtex->bottom);
            u1 = map_range(t1.u, 0, 1, subtex->left, subtex->right);
            v1 = map_range(t1.v, 0, 1, subtex->top, subtex->bottom);
            u2 = map_range(t2.u, 0, 1, subtex->left, subtex->right);
            v2 = map_range(t2.v, 0, 1, subtex->top, subtex->bottom);
        }

        C2D_DrawTriangleUV(
            x0, y0, u0, v0, C2D_Color32(color.r, color.g, color.b, opacity1), 
            x1, y1, u1, v1, C2D_Color32(color.r, color.g, color.b, opacity2), 
            x2, y2, u2, v2, C2D_Color32(color.r, color.g, color.b, opacity3), 
            0, image
        );       
    }
}

void MotionTrail_DrawWaveTrail(MotionTrail *trail) {
    change_blending(trail->blending);

    int count = trail->actualNuPoints * 2;

    Color color = trail->color;
    int opacity = 255 * trail->opacity;

    for (int i = 0; i < count - 2; i++) {
        int i0 = i;
        int i1 = i + 1;
        int i2 = i + 2;

        Vec2D p0 = trail->vertices[i0];
        Vec2D p1 = trail->vertices[i1];
        Vec2D p2 = trail->vertices[i2];

        float x0 = get_mirror_x((p0.x - state.camera_x), state.mirror_factor);
        float y0 = SCREEN_HEIGHT - ((p0.y - state.camera_y));

        float x1 = get_mirror_x((p1.x - state.camera_x), state.mirror_factor);
        float y1 = SCREEN_HEIGHT - ((p1.y - state.camera_y));

        float x2 = get_mirror_x((p2.x - state.camera_x), state.mirror_factor);
        float y2 = SCREEN_HEIGHT - ((p2.y - state.camera_y));

        C2D_DrawTriangle(
            x0, y0, C2D_Color32(color.r, color.g, color.b, opacity),
            x1, y1, C2D_Color32(color.r, color.g, color.b, opacity),
            x2, y2, C2D_Color32(color.r, color.g, color.b, opacity),
            0
        );
    }

    if (!trail->blending) return;

    // Draw center
    for (int i = 0; i < count - 2; i++) {
        int i0 = i;
        int i1 = i + 1;
        int i2 = i + 2;

        Vec2D p0 = trail->centerVertices[i0];
        Vec2D p1 = trail->centerVertices[i1];
        Vec2D p2 = trail->centerVertices[i2];

        float x0 = get_mirror_x((p0.x - state.camera_x), state.mirror_factor);
        float y0 = SCREEN_HEIGHT - ((p0.y - state.camera_y));

        float x1 = get_mirror_x((p1.x - state.camera_x), state.mirror_factor);
        float y1 = SCREEN_HEIGHT - ((p1.y - state.camera_y));

        float x2 = get_mirror_x((p2.x - state.camera_x), state.mirror_factor);
        float y2 = SCREEN_HEIGHT - ((p2.y - state.camera_y));

        C2D_DrawTriangle(
            x0, y0, C2D_Color32(165, 165, 165, opacity),
            x1, y1, C2D_Color32(165, 165, 165, opacity),
            x2, y2, C2D_Color32(165, 165, 165, opacity),
            0
        );
    }
}
