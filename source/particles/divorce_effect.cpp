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
    this->x = x;
    this->y = y;
    this->rotation = rotation;
    this->scale = scale;
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
        .lineThickness = 1.f
    };

    initUseEffectPool(&this->circle, 1);
    addUseEffect(x, y, 0, &divorceEffect, &this->circle);
}

void DivorceEffect::updateDivorce(){
    this->time += delta;

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

    float t = (time / 0.4f);
    float ti = 1.0f - t;

    spawnIconAt(
        this->gamemode, 
        selectedIcon, false, 
        getMirrorX((this->x - state.cameraX), state.mirrorFactor), 
        (this->y - state.cameraY), 
        (this->rotation) + (180.0f * t), 
        (state.mirrorMult < 0),
        this->flipY,
        this->scale * ti,
        (p1Color.toColor32() & 0x00FFFFFF) | ((u32)(ti * 255.0f) << 24), 
        (p2Color.toColor32() & 0x00FFFFFF) | ((u32)(ti * 255.0f) << 24), 
        0, 
        (IconParameters) { 
            .animID = ROBOT_ANIM_IDLE,
            .animFrame = 0
        }
    );
}

bool DivorceEffect::done(){
    return this->time > 0.4f;
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