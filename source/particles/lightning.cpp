#include "lightning.hpp"
#include <citro2d.h>
#include <random>
#include "math_helpers.hpp"
#include "graphics.hpp"
#include "state.hpp"
#include "utils/gfx.hpp"

constexpr u32 lightning_size_mini_col = C2D_Color32f(1.0f, 31 / 255.f, 1.0f, 1.0f);
constexpr u32 lightning_size_normal_col = C2D_Color32f(0.0f, 1.0f, 50.f / 255.f, 1.0f);

constexpr u32 lightning_tele_in_col = C2D_Color32f(56 / 255.f, 200 / 255.f, 1.0f, 1.0f);
constexpr u32 lightning_tele_out_col = C2D_Color32f(1.0f, 1.0f, 0.0f, 1.0f);

LightningPoint::LightningPoint(float x, float y){
    this->x = x;
    this->y = y;
}

LightningSegment::LightningSegment(float start_x, float start_y, float end_x, float end_y, u32 color){
    this->start_x = start_x;
    this->start_y = start_y;
    this->end_x = end_x;
    this->end_y = end_y;
    this->color = color;
}

static unsigned int lightning_rand(unsigned long long& seed){
    seed = seed * 1103515245 + 12345;
    return (unsigned)(seed / 65536) % 32768;
}

std::vector<LightningSegment> Lightning::generate_lighting_recursive(
    const std::vector<LightningPoint>& points, 
    int segment_length, 
    LightningType type,
    unsigned long long seed
){
    std::vector<LightningPoint> next_points;

    unsigned int i = 0;
    for(; i < points.size() - 1; i++){
        next_points.push_back(points[i]);

        float mid_x = (points[i].x + points[i + 1].x) / 2.0f;
        float mid_y = (points[i].y + points[i + 1].y) / 2.0f;
        mid_x += ((lightning_rand(seed) % 101) / 100.0f - 0.5f) * segment_length;
        mid_y += ((lightning_rand(seed) % 101) / 100.0f - 0.5f) * segment_length;

        LightningPoint point = LightningPoint(mid_x, mid_y);

        next_points.push_back(point);
    }
    next_points.push_back(points.back());

    if(segment_length > 8){
        return generate_lighting_recursive(next_points, segment_length / 2, type, seed);
    } else{
        //generate lightning
        std::vector<LightningSegment> segments;
        unsigned int segment_count = next_points.size() - 1;
        segments.reserve(segment_count);

        for(unsigned int i = 0; i < segment_count; i++){
            u32 color = 0;
            switch(type){
                case LightningType::SIZE_MINI:
                    color = lightning_size_mini_col;
                    break;
                case LightningType::SIZE_NORMAL:
                    color = lightning_size_normal_col;
                    break;
                case LightningType::TELEPORT:
                    color = p1_color.to_color32();
                    break;
                default:
                    break;
            }

            segments.push_back(LightningSegment(
                next_points[i].x, 
                next_points[i].y, 
                next_points[i + 1].x,
                next_points[i + 1].y,
                color
            ));
        }

        return segments;
    }
}

void Lightning::render_lightning(){
    for(unsigned int i = 0; i < this->segments.size(); i++){
        LightningSegment s = segments[i];

        //set alpha
        float alpha = 1.0f - (this->pulseTime * 2.0f);

        alpha = clampf(alpha, 0.0f, 1.0f);

        u32 color = (s.color & 0x00FFFFFF) |((u32)(alpha * 255.0f) << 24);

        float top_x = s.start_x - state.camera_x;
        float btm_x = s.end_x - state.camera_x;
        float top_y = SCREEN_HEIGHT - (s.start_y - state.camera_y);
        float btm_y = SCREEN_HEIGHT - (s.end_y  - state.camera_y);

        C2D_DrawLine(top_x, top_y, color, btm_x, btm_y, color, 2, 0.0f);
    }

    if(!game_paused) this->pulseTime += delta;
}

bool Lightning::done(){
    return pulseTime >= 0.5f;
}

Lightning::Lightning(float start_x, float start_y, float end_x, float end_y, LightningType type){
    std::vector<LightningPoint> segmentPoints = {
        LightningPoint(start_x, start_y),
        LightningPoint(end_x, end_y)
    };

    this->segments = generate_lighting_recursive(segmentPoints, 160, type, rand());
}

std::deque<Lightning> lightning_bolts;

void draw_all_lightning(){
    while (!lightning_bolts.empty() && lightning_bolts.front().done()) {
        lightning_bolts.pop_front();
    }

    for(auto& bolt : lightning_bolts){
        bolt.render_lightning();
    }
}