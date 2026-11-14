#pragma once
#include <array>

namespace gd3ds {

enum class ShineType {
    PortalGamemode,
    PortalGravity,
    PortalMirror,
    PortalSize,
    PortalDual,
    PortalTeleport,
    PortalSpeedSlow,
    PortalSpeedNormal,
    PortalSpeedFast,
    PortalSpeedFaster,
    PortalSpeedFastest,
    Count
};

class PortalShine {
public:
    PortalShine(ShineType type, int obj);
    void updateShine();
    void drawShine();
    bool done();
private:
    ShineType m_type;
    int m_obj;
    float m_time;
};

void updateShines();
void drawShines();
void addShine(int obj);
void resetDivorces();

}