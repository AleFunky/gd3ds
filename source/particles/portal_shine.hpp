#pragma once
#include <array>

namespace gd3ds {

enum class ShineType {
    Gamemode,
    Gravity,
    Mirror,
    Size,
    Dual,
    Teleport,
    SpeedSlow,
    SpeedNormal,
    SpeedFast,
    SpeedFaster,
    SpeedFastest,
    Count
};

class PortalShine {
public:
    PortalShine(ShineType type, int obj);
    void updateShine(float delta);
    void drawShine(bool front);
    bool done();
private:
    ShineType m_type;
    int m_obj;
    float m_time;
};

void updateShines(float delta);
void drawShines(bool front);
void addShine(ShineType type, int obj);
void resetShines();

}