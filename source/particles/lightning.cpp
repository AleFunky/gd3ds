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
    this->vertical_edges = abs(end_x - start_x) > abs(end_y - start_y);
}

std::vector<LightningSegment> Lightning::generate_lighting_recursive(
    const std::vector<LightningPoint>& points, 
    int segment_length, 
    LightningType type
){
    std::vector<LightningPoint> next_points;

    //???
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> rand_offset(-segment_length * 0.3, segment_length * 0.3); 

    unsigned int i = 0;
    for(; i < points.size() - 1; i++){
        next_points.push_back(points[i]);

        float mid_x = (points[i].x + points[i + 1].x) / 2.0f;
        float mid_y = (points[i].y + points[i + 1].y) / 2.0f;
        mid_x += rand_offset(gen);
        mid_y += rand_offset(gen);

        LightningPoint point = LightningPoint(mid_x, mid_y);

        next_points.push_back(point);
    }
    next_points.push_back(points.back());

    segment_length /= 2;
    if(segment_length > 8){
        return generate_lighting_recursive(next_points, segment_length, type);
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
                    color = color_lerp_u32(lightning_tele_in_col, lightning_tele_out_col, i / (float)segment_count);
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

        float offset_x = (s.vertical_edges ? 0 : 2);
        float offset_y = (s.vertical_edges ? 2 : 0);

        float top_l_x = s.start_x - offset_x - state.camera_x;
        float top_r_x = s.start_x + offset_x - state.camera_x;
        float btm_l_x = s.end_x - offset_x - state.camera_x;
        float btm_r_x = s.end_x + offset_x - state.camera_x;

        float top_l_y = SCREEN_HEIGHT - ((s.start_y - offset_y) - state.camera_y);
        float top_r_y = SCREEN_HEIGHT - ((s.start_y + offset_y) - state.camera_y);
        float btm_l_y = SCREEN_HEIGHT - ((s.end_y - offset_y) - state.camera_y);
        float btm_r_y = SCREEN_HEIGHT - ((s.end_y + offset_y) - state.camera_y);

        C2D_DrawTriangle(
            get_mirror_x(top_l_x, state.mirror_factor), top_l_y, color, 
            get_mirror_x(btm_l_x, state.mirror_factor), btm_l_y, color,
            get_mirror_x(btm_r_x, state.mirror_factor), btm_r_y, color,
            0.0f
        );
        C2D_DrawTriangle(
            get_mirror_x(btm_r_x, state.mirror_factor), btm_r_y, color,
            get_mirror_x(top_r_x, state.mirror_factor), top_r_y, color,
            get_mirror_x(top_l_x, state.mirror_factor), top_l_y, color,
            0.0f
        );
    }

    this->pulseTime += delta;
}

bool Lightning::done(){
    return pulseTime >= 0.5f;
}

Lightning::Lightning(float start_x, float start_y, float end_x, float end_y, LightningType type){
    std::vector<LightningPoint> segmentPoints = {
        LightningPoint(start_x, start_y),
        LightningPoint(end_x, end_y)
    };

    this->segments = generate_lighting_recursive(segmentPoints, 120, type);
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