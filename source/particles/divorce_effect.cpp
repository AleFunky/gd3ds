#include "divorce_effect.hpp"
#include "main.hpp"
#include "menus/palette_kit.hpp"
#include "utils/gfx.hpp"
#include "state.hpp"
#include "player/robot_anim_data.hpp"

DivorceEffect::DivorceEffect(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY){
    this->player = player;
    this->gamemode = gamemode;
    this->x = x;
    this->y = y;
    this->rotation = rotation;
    this->scale = scale;
    this->flipY = flipY;

    const UseEffectDefinition divorce_effect = {
        .colorR = p1_color.r / 255.0f, 
        .colorG = p1_color.g / 255.0f,
        .colorB = p1_color.b / 255.0f,
        .duration = 0.4f,
        .start_opacity = .7f,
        .end_opacity = 0,
        .start_rad = 15,
        .end_rad = 30,
        .hollow = true,
        .trifading = false,
        .line_thickness = 1.f
    };

    init_use_effect_pool(&this->circle, 1);
    add_use_effect(x, y, 0, &divorce_effect, &this->circle);
}

void DivorceEffect::update_divorce(){
    this->time += delta;

    update_use_effects(delta, &this->circle);
}

void DivorceEffect::draw_divorce(){
    change_blending(true);
    draw_use_effects(&this->circle);
    change_blending(false);
    
    int selected_icon = 0;
    switch(this->gamemode){
        case GAMEMODE_PLAYER:
            selected_icon = selected_cube;
            break;
        case GAMEMODE_SHIP:
            selected_icon = selected_ship;
            break;
        case GAMEMODE_BALL:
            selected_icon = selected_ball;
            break;
        case GAMEMODE_UFO:
            selected_icon = selected_ufo;
            break;
        case GAMEMODE_WAVE:
            selected_icon = selected_wave;
            break;
        case GAMEMODE_ROBOT:
            selected_icon = selected_robot;
            break;
    }

    float t = (time / 0.4f);
    float ti = 1.0f - t;

    spawn_icon_at(
        this->gamemode, 
        selected_icon, false, 
        get_mirror_x((this->x - state.camera_x), state.mirror_factor), 
        (this->y - state.camera_y), 
        (this->rotation) + (180.0f * t), 
        (state.mirror_mult < 0),
        this->flipY,
        this->scale * ti,
        (p1_color.to_color32() & 0x00FFFFFF) | ((u32)(ti * 255.0f) << 24), 
        (p2_color.to_color32() & 0x00FFFFFF) | ((u32)(ti * 255.0f) << 24), 
        0, 
        (IconParameters) { 
            .robot_anim_id = ROBOT_ANIM_IDLE,
            .robot_anim_frame = 0
        }
    );
}

bool DivorceEffect::done(){
    return this->time > 0.4f;
}

DivorceEffect::~DivorceEffect() {
    if(this->circle.pool){
        free(this->circle.pool);
    }
}

std::deque<DivorceEffect> divorces;

void update_divorces(){
    while (!divorces.empty() && divorces.front().done()) {
        divorces.pop_front();
    }

    for(auto& divorce : divorces){
        divorce.update_divorce();
    }
}

void draw_divorces(){
    for(auto& divorce : divorces){
        divorce.draw_divorce();
    }
}

void add_divorce(int player, int gamemode, float x, float y, float rotation, float scale, bool flipY){
    divorces.push_back(DivorceEffect(player, gamemode, x, y, rotation, scale, flipY));
}

void reset_divorces(){
    divorces.clear();
    divorces.shrink_to_fit();
}