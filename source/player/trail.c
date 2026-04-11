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

// Adds "thickness" to a line strip by generating a triangle strip
void ccVertexLineToPolygon(const Vec2* points, float stroke, Vec2* outVerts, int offset, int count) {
    if (count <= 0) return;

    float halfStroke = stroke / 2.0f;

    for (int i = offset; i < count; ++i) {
        Vec2 p = points[i];
        Vec2 dir;

        if (i == 0) {
            dir.x = points[i + 1].x - p.x;
            dir.y = points[i + 1].y - p.y;
        } else if (i == count - 1) {
            dir.x = p.x - points[i - 1].x;
            dir.y = p.y - points[i - 1].y;
        } else {
            dir.x = points[i + 1].x - points[i - 1].x;
            dir.y = points[i + 1].y - points[i - 1].y;
        }

        // Normalize direction
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
        if (len == 0) len = 1.0f;
        dir.x /= len;
        dir.y /= len;

        // Perpendicular vector
        Vec2 perp;
        perp.x = -dir.y;
        perp.y = dir.x;

        // Offset left and right
        outVerts[i * 2].x     = p.x + perp.x * halfStroke;
        outVerts[i * 2].y     = p.y + perp.y * halfStroke;

        outVerts[i * 2 + 1].x = p.x - perp.x * halfStroke;
        outVerts[i * 2 + 1].y = p.y - perp.y * halfStroke;
    }
}
// Adds "vertical thickness" to a line strip by generating a triangle strip
void ccVertexLineToPolygonWave(const Vec2* points, float stroke, Vec2* outVerts, int offset, int count) {
    if (count <= 0) return;

    float halfStroke = stroke / 2.0f;

    for (int i = offset; i < count; ++i) {
        Vec2 p = points[i];

        // Offset vertically only (same x for both vertices)
        outVerts[i * 2].x     = p.x;
        outVerts[i * 2].y     = p.y + halfStroke;

        outVerts[i * 2 + 1].x = p.x;
        outVerts[i * 2 + 1].y = p.y - halfStroke;
    }
}

static float getDistanceSq(const Vec2* a, const Vec2* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    return dx*dx + dy*dy;
}

void MotionTrail_Clear(MotionTrail *trail) {
    trail->nuPoints = 0;
    trail->previousNuPoints = 0;
}

void MotionTrail_Init(MotionTrail* trail, float fade, float minSeg, float stroke, bool waveTrail, Color color, C2D_Image tex) {
    memset(trail, 0, sizeof(MotionTrail));
    trail->image = tex;  
    trail->maxPoints = MAX_TRAIL_POINTS;
    trail->fadeDelta = 1.0f / fade;
    trail->minSeg = minSeg * minSeg;  // Compare squared distance
    trail->stroke = stroke;
    trail->displayedColor = color;
    trail->waveTrail = waveTrail;
    trail->nuPoints = 0;
    trail->previousNuPoints = 0;
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

void MotionTrail_Update(MotionTrail* trail, float delta) {
    if (trail->waveTrail) return;
    if (!trail->startingPositionInitialized) return;

    delta *= trail->fadeDelta;

    unsigned int newIdx, newIdx2, i, i2;
    unsigned int mov = 0;

    // Fade old points
    for (i = 0; i < trail->nuPoints; i++) {
        trail->pointState[i] -= delta;
        if (trail->pointState[i] <= 0) {
            mov++;
        } else {
            newIdx = i - mov;
            if (mov > 0) {
                trail->pointState[newIdx] = trail->pointState[i];
                trail->pointVertexes[newIdx] = trail->pointVertexes[i];

                i2 = i * 2;
                newIdx2 = newIdx * 2;
                trail->vertices[newIdx2] = trail->vertices[i2];
                trail->vertices[newIdx2 + 1] = trail->vertices[i2 + 1];

                i2 *= 4;
                newIdx2 *= 4;
                memcpy(&trail->colorPointer[newIdx2], &trail->colorPointer[i2], 8);
            }

            newIdx2 = newIdx * 8;
            u8 op = (u8)(trail->pointState[newIdx] * 255.0f);
            trail->colorPointer[newIdx2 + 3] = op;
            trail->colorPointer[newIdx2 + 7] = op;
        }
    }

    trail->nuPoints -= mov;

    // Append new point
    bool append = true;
    if (trail->nuPoints >= trail->maxPoints) {
        append = false;
    } else if (trail->nuPoints > 0) {
        bool a1 = getDistanceSq(&trail->pointVertexes[trail->nuPoints - 1], &trail->positionR) < trail->minSeg;
        bool a2 = (trail->nuPoints == 1) ? false : getDistanceSq(&trail->pointVertexes[trail->nuPoints - 2], &trail->positionR) < (trail->minSeg * 2.0f);
        if (a1 || a2) append = false;
    }

    if (append && trail->appendNewPoints) {
        unsigned int idx = trail->nuPoints;

        trail->pointVertexes[idx] = trail->positionR;
        trail->pointState[idx] = 1.0f;

        unsigned int offset = idx * 8;
        trail->colorPointer[offset + 0] = trail->displayedColor.r;
        trail->colorPointer[offset + 1] = trail->displayedColor.g;
        trail->colorPointer[offset + 2] = trail->displayedColor.b;
        trail->colorPointer[offset + 3] = 255;
        trail->colorPointer[offset + 4] = trail->displayedColor.r;
        trail->colorPointer[offset + 5] = trail->displayedColor.g;
        trail->colorPointer[offset + 6] = trail->displayedColor.b;
        trail->colorPointer[offset + 7] = 255;

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
        float texDelta = 1.0f / trail->nuPoints;
        for (i = 0; i < trail->nuPoints; i++) {
            trail->texCoords[i * 2].u = 0;
            trail->texCoords[i * 2].v = texDelta * i;
            trail->texCoords[i * 2 + 1].u = 1;
            trail->texCoords[i * 2 + 1].v = texDelta * i;
        }
        trail->previousNuPoints = trail->nuPoints;
    }
}
void MotionTrail_UpdateWaveTrail(MotionTrail* trail, float delta) {
    if (!trail->waveTrail) return;
    if (!trail->startingPositionInitialized) return;

    unsigned int mov = 0;
    unsigned int startIdx = 0;
    trail->offscreenCount = 0;
    
    // Update stroke width
    float size_value = 20.f * map_range(amplitude, 0.f, 1.f, 0.1f, 1.f);
    trail->stroke = size_value;
    
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

            i2 *= 4;
            newIdx2 *= 4;
            memcpy(&trail->colorPointer[newIdx2], &trail->colorPointer[i2], 8);
        }

        // Set opacity
        unsigned int colorIdx = newIdx * 8;
        trail->colorPointer[colorIdx + 3] = 255 * trail->opacity;
        trail->colorPointer[colorIdx + 7] = 255 * trail->opacity;
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

    unsigned int offset = idx * 8;
    trail->colorPointer[offset + 0] = trail->displayedColor.r;
    trail->colorPointer[offset + 1] = trail->displayedColor.g;
    trail->colorPointer[offset + 2] = trail->displayedColor.b;
    trail->colorPointer[offset + 3] = 255;
    trail->colorPointer[offset + 4] = trail->displayedColor.r;
    trail->colorPointer[offset + 5] = trail->displayedColor.g;
    trail->colorPointer[offset + 6] = trail->displayedColor.b;
    trail->colorPointer[offset + 7] = 255;

    trail->nuPoints++;

    if (trail->nuPoints > 1) {
        ccVertexLineToPolygonWave(trail->pointVertexes, trail->stroke, trail->vertices, 0, trail->actualNuPoints);
        ccVertexLineToPolygonWave(trail->pointVertexes, trail->stroke * 0.4f, trail->centerVertices, 0, trail->actualNuPoints);
    }

    trail->previousNuPoints = trail->nuPoints;
}

void MotionTrail_Draw(MotionTrail* trail) {
    C2D_Image image = trail->image;

    int count = trail->nuPoints * 2;

    for (int i = 0; i < count - 2; i++) {
        int i0 = i;
        int i1 = i + 1;
        int i2 = i + 2;

        Vec2 p0 = trail->vertices[i0];
        Vec2 p1 = trail->vertices[i1];
        Vec2 p2 = trail->vertices[i2];

        float x0 = get_mirror_x((p0.x - state.camera_x), state.mirror_factor);
        float y0 = SCREEN_HEIGHT - ((p0.y - state.camera_y));

        float x1 = get_mirror_x((p1.x - state.camera_x), state.mirror_factor);
        float y1 = SCREEN_HEIGHT - ((p1.y - state.camera_y));

        float x2 = get_mirror_x((p2.x - state.camera_x), state.mirror_factor);
        float y2 = SCREEN_HEIGHT - ((p2.y - state.camera_y));

        u8* c0 = &trail->colorPointer[i0 * 4];
        u8* c1 = &trail->colorPointer[i1 * 4];
        u8* c2 = &trail->colorPointer[i2 * 4];

        Tex2F t0 = trail->texCoords[i0];
        Tex2F t1 = trail->texCoords[i1];
        Tex2F t2 = trail->texCoords[i2];

        const Tex3DS_SubTexture *subtex = trail->image.subtex;

        float u0 = map_range(t0.u, 0, 1, subtex->left, subtex->right);
        float v0 = map_range(t0.v, 0, 1, subtex->bottom, subtex->top);
        float u1 = map_range(t1.u, 0, 1, subtex->left, subtex->right);
        float v1 = map_range(t1.v, 0, 1, subtex->bottom, subtex->top);
        float u2 = map_range(t2.u, 0, 1, subtex->left, subtex->right);
        float v2 = map_range(t2.v, 0, 1, subtex->bottom, subtex->top);

        C2D_DrawTriangleUV(
            x0, y0, u0, v0, C2D_Color32(c0[0], c0[1], c0[2], c0[3]), 
            x1, y1, u1, v1, C2D_Color32(c1[0], c1[1], c1[2], c1[3]), 
            x2, y2, u2, v2, C2D_Color32(c2[0], c2[1], c2[2], c2[3]), 
            0, image
        );       
    }
}

void MotionTrail_DrawWaveTrail(MotionTrail *trail) {
    int count = trail->actualNuPoints * 2;

    for (int i = 0; i < count - 2; i++) {
        int i0 = i;
        int i1 = i + 1;
        int i2 = i + 2;

        Vec2 p0 = trail->vertices[i0];
        Vec2 p1 = trail->vertices[i1];
        Vec2 p2 = trail->vertices[i2];

        float x0 = get_mirror_x((p0.x - state.camera_x), state.mirror_factor);
        float y0 = SCREEN_HEIGHT - ((p0.y - state.camera_y));

        float x1 = get_mirror_x((p1.x - state.camera_x), state.mirror_factor);
        float y1 = SCREEN_HEIGHT - ((p1.y - state.camera_y));

        float x2 = get_mirror_x((p2.x - state.camera_x), state.mirror_factor);
        float y2 = SCREEN_HEIGHT - ((p2.y - state.camera_y));

        u8* c0 = &trail->colorPointer[i0 * 4];
        u8* c1 = &trail->colorPointer[i1 * 4];
        u8* c2 = &trail->colorPointer[i2 * 4];

        C2D_DrawTriangle(
            x0, y0, C2D_Color32(c0[0], c0[1], c0[2], c0[3]),
            x1, y1, C2D_Color32(c1[0], c1[1], c1[2], c1[3]),
            x2, y2, C2D_Color32(c2[0], c2[1], c2[2], c2[3]),
            0
        );
    }

    // Draw center
    for (int i = 0; i < count - 2; i++) {
        int i0 = i;
        int i1 = i + 1;
        int i2 = i + 2;

        Vec2 p0 = trail->centerVertices[i0];
        Vec2 p1 = trail->centerVertices[i1];
        Vec2 p2 = trail->centerVertices[i2];

        float x0 = get_mirror_x((p0.x - state.camera_x), state.mirror_factor);
        float y0 = SCREEN_HEIGHT - ((p0.y - state.camera_y));

        float x1 = get_mirror_x((p1.x - state.camera_x), state.mirror_factor);
        float y1 = SCREEN_HEIGHT - ((p1.y - state.camera_y));

        float x2 = get_mirror_x((p2.x - state.camera_x), state.mirror_factor);
        float y2 = SCREEN_HEIGHT - ((p2.y - state.camera_y));

        C2D_DrawTriangle(
            x0, y0, C2D_Color32(165, 165, 165, 255 * trail->opacity),
            x1, y1, C2D_Color32(165, 165, 165, 255 * trail->opacity),
            x2, y2, C2D_Color32(165, 165, 165, 255 * trail->opacity),
            0
        );
    }
}
