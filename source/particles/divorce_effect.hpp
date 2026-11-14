#pragma once
#include "circles.hpp"
#include <deque>
namespace gd3ds {

class DivorceEffect {
private:
    int player;
    int gamemode;
    float x, y;
    float rotation;
    float scale;
    bool flipX;
    bool flipY;
    bool mirrored;

    float time;
    UseEffectPool circle;
public:
    DivorceEffect(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY);

    void updateDivorce();
    void drawDivorce();
    bool done();
    ~DivorceEffect();
};

void updateDivorces();
void drawDivorces();
void addDivorce(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY);
void resetDivorces();

}