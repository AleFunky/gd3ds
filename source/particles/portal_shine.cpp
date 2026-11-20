#include "portal_shine.hpp"
#include "menus/settings_hub/settings.hpp"
#include "graphics.hpp"
#include "main.hpp"
#include "state.hpp"
#include "utils/gfx.hpp"
#include "math_helpers.hpp"
#include "level_loading.hpp"
#include <vector>
#include <array>

namespace gd3ds {

constexpr std::array<int, static_cast<int>(ShineType::Count) * 2> kShineSheetIndices = {
//back, front
    8, 9, //gamemode
    6, 7, //gravity
    10, 11, //mirror
    12, 13, //size
    14, 15, //dual
    16, 17, //teleport
    //speed portals
    1, 0,
    2, 0,
    3, 0,
    4, 0,
    5, 0
};

static int getShineSheetIndex(ShineType type, bool front){
    //array alternates between back and front
    return kShineSheetIndices[
        (static_cast<int>(type) * 2) + (front ? 1 : 0)
    ];
}

static bool hasFront(ShineType type){
    //speed portals have no front
    return static_cast<int>(type) < static_cast<int>(ShineType::SpeedSlow);
}

PortalShine::PortalShine(ShineType type, int obj){
    m_type = type;
    m_obj = obj;
    m_time = 0.0f;
}

void PortalShine::updateShine(float delta){
    m_time += delta;
}

void PortalShine::drawShine(bool front){
    if((front && !hasFront(m_type)) || objects.opacity[m_obj] <= 0.f) return;

    float fade = 0.0f;
    if(m_time <= 0.05f){
        fade = m_time / 0.05f;
    } else{
        fade = 1.0f - (m_time / 0.4f);
    }

    fade = C2D_Clamp(fade, 0.0f, 1.0f);

    float calcX = objects.x[m_obj] - state.cameraX;
    float calcY = SCREEN_HEIGHT - (objects.y[m_obj] - state.cameraY);
    float fadeVal = objEdgeFade(calcX, GAMEPLAY_SCREEN_WIDTH_AREA);
    
    float fadeX = 0;
    float fadeY = 0;
    float fadeScale = 1.f;
    getFadeVars(m_obj, calcX, &fadeX, &fadeY, &fadeScale);

    // Handle special fade types
    fadeX += getSpecialFadingVars(m_obj, fadeVal);

    float x = getMirrorX(calcX + fadeX, state.mirrorFactor);
    float y = calcY + fadeY;
    float deg = objects.visualRotation[m_obj];
    bool flipX = objects.flippedH[m_obj] ^ (state.mirrorMult < 0);
    bool flipY = objects.flippedV[m_obj];
    float scale = fadeScale;

    float objScaleX = objects.scaleX[m_obj];
    float objScaleY = objects.scaleY[m_obj];

    const GameObject* obj = (const GameObject*)&gameObjects[objects.id[m_obj]];

    float rad = C3D_AngleFromDegrees(adjustAngle(deg, 0, state.mirrorMult < 0));
    float cosR = lutCos(rad);
    float sinR = lutSin(rad);

    int flipXMult = (flipX ? -1 : 1);
    int flipYMult = (flipY ? -1 : 1);

    float m00 = cosR;
    float m01 = sinR;
    float m10 = sinR;
    float m11 = -cosR;

    float sx = scale * flipXMult * objScaleX;
    float sy = scale * flipYMult * objScaleY;

    float localX = obj->x * flipXMult;
    float localY = obj->y * flipYMult;

    //get child values
    if(hasFront(m_type) && !front){
       const ChildSprite* c = &obj->children[0];
       localX = c->x * flipXMult;
       localY = c->y * flipYMult;
    }

    C2D_Image image = C2D_SpriteSheetGetImage(miscFXSheet, getShineSheetIndex(m_type, front));
    if(flipX && !flipY){
        localX -= image.subtex->width;
    }
    if(flipY && !flipX){
        localY += image.subtex->height;
    }

    float rotX = localX * m00 + localY * m01;
    float rotY = localX * m10 + localY * m11;

    float pX = x + rotX * scale * objScaleX;
    float pY = y + rotY * scale * objScaleY;

    u32 color = C2D_Color32f(1.0f, 1.0f, 1.0f, fade);

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, color, 1.0f);

    C2D_DrawImageAtRotated(
        image, pX, pY, 0.0f, rad, &tint, sx, sy
    );
}

bool PortalShine::done(){
    //fade in and out combined
    return m_time >= 0.05f + 0.4f;
};

std::vector<PortalShine> shines;

void updateShines(float delta){
    for(auto& shine : shines){
        shine.updateShine(delta);
    }
}

void drawShines(bool front){
    for(auto& shine : shines){
        shine.drawShine(front);
    }
}

void addShine(ShineType type, int obj){
    for(auto& shine : shines){
        if(shine.done()) {
            shine = PortalShine(type, obj);
            return;
        }
    }
    shines.push_back(PortalShine(type, obj));
}

void resetShines(){
    shines.clear();
    shines.shrink_to_fit();
}

}