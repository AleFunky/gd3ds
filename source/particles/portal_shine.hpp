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
private:
    ShineType m_type;
    int m_obj;
    float m_time;
public:
    PortalShine(ShineType type, int obj);
    void updateShine();
    void drawShine();
    bool done();
};

}