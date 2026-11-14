#include "portal_shine.hpp"
#include "graphics.hpp"
#include "main.hpp"
#include "state.hpp"
#include "utils/gfx.hpp"
#include "level_loading.hpp"
#include <deque>

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
    3, 0,
    4, 0,
    5, 0,
    6, 0,
    7, 0
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
}

void PortalShine::updateShine(){
    m_time += delta;
}

void PortalShine::drawShine(bool front){
    if(front && !hasFront(m_type)) return;

    float fade = 0.0f;
    if(m_time <= 0.05f){
        fade = m_time / 0.05f;
    } else{
        fade = 1.0f - (m_time / 0.4f);
    }

    fade = C2D_Clamp(fade, 0.0f, 1.0f);
    u32 color = C2D_Color32f(1.0f, 1.0f, 1.0f, fade);

    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, color, 1.0f);

    float scaleX = objects.scaleX[m_obj];
    if(objects.flippedH[m_obj]) scaleX *= -1.0f;
    if(state.mirrorMult < 0) scaleX *= -1.0f;

    float scaleY = objects.scaleY[m_obj];
    if(objects.flippedV[m_obj]) scaleY *= -1.0f;

    C2D_DrawImageAtRotated(
        C2D_SpriteSheetGetImage(miscFXSheet, getShineSheetIndex(m_type, front)), 
        getMirrorX(objects.x[m_obj] - state.cameraX, state.mirrorFactor) * 0.0f + SCREEN_WIDTH / 2.0f, 
        (SCREEN_HEIGHT - (objects.y[m_obj] - state.cameraY)) * 0.0f + SCREEN_HEIGHT / 2.0f, 
        0.0f, 
        objects.rotation[m_obj],
        &tint, 
        scaleX, 
        scaleY
    );
}

bool PortalShine::done(){
    //fade in and out combined
    return m_time >= 0.05f + 0.4f;
};

std::deque<PortalShine> shines;

void updateShines(){
    while (!shines.empty() && shines.front().done()) {
        shines.pop_front();
    }

    for(auto& shine : shines){
        shine.updateShine();
    }
}
void drawShines(bool front){
    for(auto& shine : shines){
        shine.drawShine(front);
    }
}

void addShine(ShineType type, int obj){
    shines.push_back(PortalShine(type, obj));
}

void resetShines(){
    shines.clear();
    shines.shrink_to_fit();
}

}