#pragma once
#include "circles.hpp"
#include <deque>

class DivorceEffect {
private:
    int player;
    int gamemode;
    float x, y;
    float rotation;
    float scale;
    bool flipY;

    float time;
    UseEffectPool circle;
public:
    DivorceEffect(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY);

    void update_divorce();
    void draw_divorce();
    bool done();
    ~DivorceEffect();
};

void update_divorces();
void draw_divorces();
void add_divorce(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY);
void reset_divorces();