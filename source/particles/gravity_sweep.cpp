#include "gravity_sweep.hpp"

#include "main.hpp"
#include "math_helpers.hpp"
#include "graphics.hpp"
#include "menus/settings_hub/settings.hpp"
#include "game/game_manager.hpp"

#include "constants.hpp"

namespace gd3ds
{

GravitySweep::GravitySweep(bool down){
    m_down = down;
    m_time = 9999.0f;
};

void GravitySweep::updateSweepTimer(){
    if(done()) return;
    m_time += GameManager::get()->deltaTime;
}

void GravitySweep::startSweep(){
    m_time = 0.0f;
    float spacing = (constants::ScreenWidth - 30.0f) / 22.0f;
    for(int i = 0; i < 22; i++){
        GravityLine* l = &this->m_lines[i];
        l->x = (i * spacing) + (CCRANDOM_0_1() * spacing) + 15.0f;
        l->y = (CCRANDOM_0_1() + CCRANDOM_0_1() - 1.0f) * 10.0f;
        l->scaleX = (CCRANDOM_0_1() * 4.0f) + 2.0f;
        l->scaleY = CCRANDOM_0_1() + 2.0f;
        l->opacity = ((CCRANDOM_0_1() * 130.0f) + 50.0f) / 255.0f;
    }
}


void GravitySweep::renderSweep(){
    if(done()) return;

    float t = clampf(m_time / 0.4f, 0.0f, 1.0f);
    float ti = (1.0f - t);

    float yOffset;
    if(m_down){
        yOffset = t * ((constants::ScreenHeight / 2.0f) + 95.0f) + ti * ((-constants::ScreenHeight / 2.0f) - 95.0f);
    } else{
        yOffset = ti * ((constants::ScreenHeight / 2.0f) + 95.0f) + t * ((-constants::ScreenHeight / 2.0f) - 95.0f);
    }

    for(const auto& l : m_lines){
        u32 color;
        if(m_down){
            color = C2D_Color32f(0.0f, 200.0f / 255.0f, 1.0f, l.opacity);
        } else{
            color = C2D_Color32f(1.0f, 200.0f / 255.0f, 0.0f, l.opacity);
        }

        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, color, 1.0f);

        C2D_DrawImageAt(
            C2D_SpriteSheetGetImage(miscFXSheet, 0), 
            l.x, l.y + yOffset, 0.0f, &tint, l.scaleX, l.scaleY
        );
    }
};

bool GravitySweep::done(){
    return m_time > 0.4f;
}

GravitySweep upSweeps[2] = {
    GravitySweep(false),
    GravitySweep(false)
};
GravitySweep downSweeps[2] = {
    GravitySweep(true),
    GravitySweep(true)
};

void startGravitySweep(bool down){
    if(settingsState.disableGravityEffect) return;

    auto& sweeps = down ? downSweeps : upSweeps;
    for(auto& s : sweeps){
        if(s.done()){
            s.startSweep(); 
            break;
        }
    }
}

void updateGravitySweeps(){
    for(int i = 0; i < 2; i++){
        upSweeps[i].updateSweepTimer();
        downSweeps[i].updateSweepTimer();
    }
}

void drawGravitySweeps(){
    //temporary screen space wow
    C2D_ViewTranslate(0, -CAM_Y_MTX_OFFSET);
    C2D_ViewScale(1/SCALE, 1/SCALE);

    for(int i = 0; i < 2; i++){
        upSweeps[i].renderSweep();
        downSweeps[i].renderSweep();
    }

    C2D_ViewScale(SCALE, SCALE);
    C2D_ViewTranslate(0, CAM_Y_MTX_OFFSET);
}

void resetGravitySweeps(){
    for(int i = 0; i < 2; i++){
        upSweeps[i] = GravitySweep(false);
        downSweeps[i] = GravitySweep(true);
    }
}

}