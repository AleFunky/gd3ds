#include "gravity_sweep.hpp"
#include "main.hpp"
#include "math_helpers.hpp"
#include "graphics.hpp"
#include <array>

C2D_SpriteSheet gravLineSheet;

GravitySweep::GravitySweep(bool down){
    this->down = down;
    this->time = 9999.0f;
};

bool GravitySweep::done(){
    return this->time > 0.4f;
}

void GravitySweep::render_sweep(){
    if(this->done()) return;

    float t = clampf(this->time / 0.4f, 0.0f, 1.0f);
    float ti = (1.0f - t);

    float y_offset;
    if(this->down){
        y_offset = t * ((SCREEN_HEIGHT / 2.0f) + 60.0f) + ti * ((-SCREEN_HEIGHT / 2.0f) - 60.0f);
    } else{
        y_offset = ti * ((SCREEN_HEIGHT / 2.0f) + 60.0f) + t * ((-SCREEN_HEIGHT / 2.0f) - 60.0f);
    }

    for(const auto& l : this->lines){
        u32 color;
        if(this->down){
            color = C2D_Color32f(0.0f, 200.0f / 255.0f, 1.0f, l.opacity);
        } else{
            color = C2D_Color32f(1.0f, 200.0f / 255.0f, 0.0f, l.opacity);
        }

        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, color, 1.0f);

        C2D_DrawImageAt(
            C2D_SpriteSheetGetImage(gravLineSheet, 0), 
            l.x, l.y + y_offset, 0.0f, &tint, l.scaleX, l.scaleY);
    }

    if(!game_paused) this->time += delta;
};

void GravitySweep::start_sweep(){
    this->time = 0.0f;
    float spacing = (SCREEN_WIDTH - 30.0f) / 22.0f;
    for(int i = 0; i < 22; i++){
        GravityLine* l = &this->lines[i];
        l->x = (i * spacing) + (CCRANDOM_0_1() * spacing) + 15.0f;
        l->y = (CCRANDOM_0_1() + CCRANDOM_0_1() - 1.0f) * 10.0f;
        l->scaleX = (CCRANDOM_0_1() * 4.0f) + 2.0f;
        l->scaleY = CCRANDOM_0_1() + 2.0f;
        l->opacity = ((CCRANDOM_0_1() * 130.0f) + 50.0f) / 255.0f;
    }
}

std::array<GravitySweep, 2> up_sweeps = {
    GravitySweep(false),
    GravitySweep(false)
};
std::array<GravitySweep, 2> down_sweeps = {
    GravitySweep(true),
    GravitySweep(true)
};

void start_gravity_sweep(bool down){
    auto& sweeps = down ? down_sweeps : up_sweeps;
    for(auto& s : sweeps){
        if(s.done()){
            s.start_sweep(); 
            break;
        }
    }
}

void draw_gravity_sweeps(){
    //temporary screen space wow
    C2D_ViewTranslate(0, -CAM_Y_MTX_OFFSET);
    C2D_ViewScale(1/SCALE, 1/SCALE);

    for(auto& sweep: up_sweeps){
        sweep.render_sweep();
    }

    for(auto& sweep: down_sweeps){
        sweep.render_sweep();
    }

    C2D_ViewScale(SCALE, SCALE);
    C2D_ViewTranslate(0, CAM_Y_MTX_OFFSET);
}