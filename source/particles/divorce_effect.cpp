#include "divorce_effect.hpp"
#include "main.hpp"
#include "menus/palette_kit.hpp"
#include "utils/gfx.hpp"
#include "state.hpp"
#include "player/robot_anim_data.hpp"
namespace gd3ds {

DivorceEffect::DivorceEffect(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY){
    this->player = player;
    this->gamemode = gamemode;
    //initial X position should be in world space, but not the subsequent animation (robert code)
    this->x = getMirrorX(x - state.cameraX, state.mirrorFactor);
    this->y = y;
    this->rotation = rotation;
    this->scale = scale;
    this->mirrored = (state.mirrorMult < 0);
    this->flipX = this->mirrored;
    this->flipY = flipY;

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
        .startOpacityEase = EASE_IN,
        .endOpacityEase = EASE_OUT,
        .startRadEase = QUAD_OUT,
        .endRadEase = QUAD_OUT,
        .lineThickness = 1.f
    };

    initUseEffectPool(&this->circle, 1);
    addUseEffect(0, 0, USE_EFFECT_OBJ_NOTHING, &divorceEffect, &this->circle);
    this->circle.stationary = true;
}

void DivorceEffect::updateDivorce(){
    this->time += delta;

    //it only moves relative to 1x speed for some reason
    this->x -= this->mirrored ? -playerSpeeds[1] * delta : playerSpeeds[1] * delta;

    this->circle.pool[0].x = this->x;
    this->circle.pool[0].y = SCREEN_HEIGHT - (this->y - state.cameraY);

    updateUseEffects(delta, &this->circle);
}

void DivorceEffect::drawDivorce(){
    changeBlending(true);
    drawUseEffects(&this->circle);
    changeBlending(false);
    
    int selectedIcon = 0;
    switch(this->gamemode){
        case GAMEMODE_PLAYER:
            selectedIcon = selectedCube;
            break;
        case GAMEMODE_SHIP:
            selectedIcon = selectedShip;
            break;
        case GAMEMODE_BALL:
            selectedIcon = selectedBall;
            break;
        case GAMEMODE_UFO:
            selectedIcon = selectedUfo;
            break;
        case GAMEMODE_WAVE:
            selectedIcon = selectedWave;
            break;
        case GAMEMODE_ROBOT:
            selectedIcon = selectedRobot;
            break;
    }

    float t = (time / 0.5f);
    float ti = 1.0f - t;

    Color p1 = p1Color;
    Color p2 = p2Color;
    if(this->player == 1){
        p1 = p2Color;
        p2 = p1Color;
    }

    spawnIconAt(
        this->gamemode, 
        selectedIcon, false, 
        this->x, 
        SCREEN_HEIGHT - (this->y - state.cameraY), 
        (this->rotation) + (180.0f * t), 
        this->flipX,
        this->flipY,
        this->scale * ti,
        (p1.toColor32() & 0x00FFFFFF) | ((u32)(ti * 255.0f) << 24), 
        (p2.toColor32() & 0x00FFFFFF) | ((u32)(ti * 255.0f) << 24), 
        0, 
        (IconParameters) { 
            .animID = ROBOT_ANIM_IDLE,
            .animFrame = 0
        }
    );
}

bool DivorceEffect::done(){
    return this->time > 0.5f;
}

DivorceEffect::~DivorceEffect() {
    if(this->circle.pool){
        free(this->circle.pool);
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