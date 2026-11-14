#pragma once
#include <array>

namespace gd3ds {

enum ShineType {
    PORTAL_GAMEMODE,
    PORTAL_GRAVITY,
    PORTAL_MIRROR,
    PORTAL_SIZE,
    PORTAL_DUAL,
    PORTAL_TELEPORT,
    PORTAL_SPEED_SLOW,
    PORTAL_SPEED_NORMAL,
    PORTAL_SPEED_FAST,
    PORTAL_SPEED_FASTER,
    PORTAL_SPEED_FASTEST,
    PORTAL_SHINE_COUNT
};

class PortalShine {
private:
    ShineType type;
    int obj;
    float time;
public:
    PortalShine(ShineType type, int obj);
    void updateShine();
    void drawShine();
    bool done();
};

}