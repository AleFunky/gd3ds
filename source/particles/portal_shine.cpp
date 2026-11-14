#include "portal_shine.hpp"
#include "main.hpp"
#include "level_loading.hpp"

namespace gd3ds {

const std::array<int, PORTAL_SHINE_COUNT * 2> shineSheetIndices = {
//back, front
    8, 9, //gamemode
    6, 7, //gravity
    10, 11, //mirror
    12, 13, //size
    14, 15, //dual
    16, 17, //teleport
    //speed portals
    3, -1,
    4, -1,
    5, -1,
    6, -1
};

static int getShineSheetIndex(ShineType type, bool front){
    //array alternates between back and front
    return shineSheetIndices[(type * 2) + (front ? 1 : 0)];
}

PortalShine::PortalShine(ShineType type, int obj){
    this->type = type;
    this->obj = obj;
}

void PortalShine::updateShine(){
    this->time += delta;
}

void PortalShine::drawShine(){

}

}