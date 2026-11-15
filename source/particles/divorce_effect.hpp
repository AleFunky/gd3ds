#pragma once
#include "circles.hpp"
namespace gd3ds {

class DivorceEffect {
public:
    DivorceEffect(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY);

    void updateDivorce();
    void drawDivorce();
    bool done();
    ~DivorceEffect();
private:
    int m_player;
    int m_gamemode;
    float m_x, m_y;
    float m_rotation;
    float m_scale;
    bool m_flipY;
    bool m_mirrored;

    float m_time;
    UseEffectPool m_circle;
};

void updateDivorces();
void drawDivorces();
void addDivorce(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY);
void resetDivorces();

}