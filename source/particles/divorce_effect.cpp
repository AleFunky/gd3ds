#include "divorce_effect.hpp"
#include "main.hpp"
#include "menus/palette_kit.hpp"
#include "utils/gfx.hpp"
#include "state.hpp"
#include "player/robot_anim_data.hpp"
#include "player/spider_anim_data.hpp"
namespace gd3ds {

DivorceEffect::DivorceEffect(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY){
    m_player = player;
    m_gamemode = gamemode;
    //initial X position should be in world space, but not the subsequent animation (robert code)
    m_x = getMirrorX(x - state.cameraX, state.mirrorFactor);
    m_y = SCREEN_HEIGHT - (y - state.cameraY);
    m_rotation = rotation;
    m_scale = scale;
    m_mirrored = (state.mirrorMult < 0);
    m_flipY = flipY;

    const UseEffectDefinition divorceEffect = {
        .colorR = p1Color.r / 255.0f, 
        .colorG = p1Color.g / 255.0f,
        .colorB = p1Color.b / 255.0f,
        .duration = 0.4f,
        .startOpacity = .7f,
        .endOpacity = 0,
        .startRad = 15,
        .endRad = 30,
        .hollow = true,
        .trifading = false,
        .startOpacityEase = EaseTypes::EaseIn,
        .endOpacityEase = EaseTypes::EaseOut,
        .startRadEase = EaseTypes::QuadOut,
        .endRadEase = EaseTypes::QuadOut,
        .lineThickness = 1.f
    };

    initUseEffectPool(&m_circle, 1);
    addUseEffect(0, 0, USE_EFFECT_OBJ_NOTHING, &divorceEffect, &m_circle);
    m_circle.stationary = true;
}

void DivorceEffect::updateDivorce(){
    m_time += delta;

    //it only moves relative to 1x speed for some reason
    m_x -= m_mirrored ? -playerSpeeds[1] * delta : playerSpeeds[1] * delta;

    m_circle.pool[0].x = m_x;
    m_circle.pool[0].y = m_y;

    updateUseEffects(delta, &m_circle);
}

void DivorceEffect::drawDivorce(){
    changeBlending(true);
    drawUseEffects(&m_circle);
    changeBlending(false);
    
    int selectedIcon = 0;
    switch(static_cast<IconGamemode>(m_gamemode)){
        case IconGamemode::GamemodePlayer:
            selectedIcon = selectedCube;
            break;
        case IconGamemode::GamemodeShip:
            selectedIcon = selectedShip;
            break;
        case IconGamemode::GamemodeBall:
            selectedIcon = selectedBall;
            break;
        case IconGamemode::GamemodeUfo:
            selectedIcon = selectedUfo;
            break;
        case IconGamemode::GamemodeWave:
            selectedIcon = selectedWave;
            break;
        case IconGamemode::GamemodeRobot:
            selectedIcon = selectedRobot;
            break;
        case IconGamemode::GamemodeSpider:
            selectedIcon = selectedSpider;
            break;
        default:
            break;
    }

    float t = (m_time / 0.4f);
    float ti = 1.0f - t;

    Color p1 = p1Color;
    Color p2 = p2Color;
    if(m_player == 1){
        p1 = p2Color;
        p2 = p1Color;
    }

    spawnIconAt(
        m_gamemode, 
        selectedIcon, false, 
        m_x, 
        m_y, 
        (m_rotation) + (180.0f * t), 
        m_mirrored,
        m_flipY,
        m_scale * ti,
        (p1.toColor32() & 0x00FFFFFF) | ((u32)(ti * 255.0f) << 24), 
        (p2.toColor32() & 0x00FFFFFF) | ((u32)(ti * 255.0f) << 24), 
        0, 
        (IconParameters) { 
            .animID = (m_gamemode == (int)static_cast<int>(IconGamemode::GamemodeRobot) ? (int)static_cast<int>(RobotAnimId::RobotAnimIdle) : (int)static_cast<int>(SpiderAnimId::SpiderAnimIdle)),
            .animFrame = 0
        }
    );
}

bool DivorceEffect::done(){
    return m_time > 0.4f;
}

DivorceEffect::~DivorceEffect() {
    if(m_circle.pool){
        free(m_circle.pool);
    }
}

std::deque<DivorceEffect> divorces;

void updateDivorces(){
    while (!divorces.empty() && divorces.front().done()) {
        divorces.pop_front();
    }

    for(auto& divorce : divorces){
        divorce.updateDivorce();
    }
}

void drawDivorces(){
    for(auto& divorce : divorces){
        divorce.drawDivorce();
    }
}

void addDivorce(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY){
    divorces.push_back(DivorceEffect(player, gamemode, x, y, rotation, scale, flipY));
}

void resetDivorces(){
    divorces.clear();
    divorces.shrink_to_fit();
}

}