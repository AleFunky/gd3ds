#include "menus/core/common_setters.h"

#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "ui_slider.h"

// If currently using an slider
bool sliding;

static void draw_button(UISlider *e, UITransform *transform) {
    float width = e->base.w * transform->scaleX;
    float left_side = transform->x - (width / 2);
    
    float percent = ui_slider_get_percent(e);
    int button_x = left_side + (int)(percent * width);

    C2D_SpriteSetCenter(&e->button, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->button, button_x, transform->y);
    C2D_SpriteSetScale(&e->button, transform->scaleX, transform->scaleY);
    C2D_DrawSprite(&e->button);
}

static void draw_frame(UISlider *e, UITransform *transform) {
    C2D_SpriteSetCenter(&e->track_frame, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->track_frame, transform->x, transform->y);
    C2D_SpriteSetScale(&e->track_frame, transform->scaleX, transform->scaleY);
    C2D_DrawSprite(&e->track_frame);
}

static void draw_bar(UISlider *e, UITransform *transform) {
    float bar_width = e->track.image.subtex->width;
    
    int pixels = (e->value / e->max_value) * bar_width;

    if (pixels > 0) {
        if (pixels > bar_width) {
            pixels = bar_width;
        }

        C2D_Sprite spr;
        Tex3DS_SubTexture sub;
        C2D_Image img;
        C2D_ImageTint tint;
        
        float x = transform->x - (e->base.w / 2) * transform->scaleX;
        
        C2D_PlainImageTint(&tint, C2D_Color32(50, 190, 240, 255), 1.f);

        sub = select_box(&e->track.image, 0, 0, pixels, e->track.image.subtex->height);
        img = e->track.image; img.subtex = &sub;

        C2D_SpriteFromSheet(&e->button, bar_sheet, 4 + e->dragging);
        C3D_TexSetFilter(e->button.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteFromImage(&spr, img);
        C2D_SpriteSetCenter(&spr, 0.f, 0.5f);
        C2D_SpriteSetPos(&spr, x, transform->y);
        C2D_SpriteSetScale(&spr, transform->scaleX, transform->scaleY);
        C2D_DrawSpriteTinted(&spr, &tint);
    }
}

static float slider_button_x(UISlider* e, UITransform *transform) {
    float width = e->base.w * transform->scaleX;
    float left_side = transform->x - width / 2;

    float t = e->value / e->max_value;

    return left_side + (t * width);
}

static void slider_set_from_x(UISlider* e, UITransform *transform, float px) {
    float width = e->base.w * transform->scaleX;
    float left_side = transform->x - width / 2;

    float t = (float)(px - left_side) / width;

    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;

    e->value = t * e->max_value;
}

static bool slider_touching_button(UISlider* e, UIInput* touch, UITransform *transform) {
    int button_x = slider_button_x(e, transform);

    float width = e->button.image.subtex->width * transform->scaleX;
    float height = e->button.image.subtex->height * transform->scaleY;

    return touch->touchPosition.px >= button_x - width / 2 &&
           touch->touchPosition.px <= button_x + width / 2 &&
           touch->touchPosition.py >= transform->y - height / 2 &&
           touch->touchPosition.py <= transform->y + height / 2;
}

void ui_slider_set_value(UISlider *e, float value) {
    if (!e) return;

    e->value = value;
}

float ui_slider_get_percent(UISlider* e) {
    if (!e) return 0;

    return e->value / e->max_value;
}

static void ui_slider_update(UIElement* e, UIInput* touch, UITransform *transform) {
    UISlider* s = (UISlider *) e;

    bool inside = slider_touching_button(s, touch, transform);

    bool pressedTouch = touch->down & KEY_TOUCH;
    bool heldTouch = touch->held & KEY_TOUCH;
    bool releasedTouch = touch->up & KEY_TOUCH;

    if (pressedTouch && inside) {
        s->dragging = true;
        sliding = true;
    }

    if (s->dragging && heldTouch) {
        slider_set_from_x(s, transform, touch->touchPosition.px);
    }

    if (releasedTouch) {
        s->dragging = false;
        sliding = false;
    }

    // Mask background elements
    if (inside) {
        touch->interacted = true;
    }
}

static void ui_slider_draw(UIElement* e, UITransform *transform) {
    UISlider* s = (UISlider *) e;

    // Track
    draw_bar(s, transform);
    draw_frame(s, transform);

    // Button
    draw_button(s, transform);
}

static void ui_slider_destroy(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

static void ui_slider_init_graphics(UISlider *e) {
    C2D_SpriteFromSheet(&e->track, bar_sheet, 3);
    C3D_TexSetFilter(e->track.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteFromSheet(&e->track_frame, bar_sheet, 1);
    C3D_TexSetFilter(e->track_frame.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteFromSheet(&e->button, bar_sheet, 4);
    C3D_TexSetFilter(e->button.image.tex, GPU_LINEAR, GPU_LINEAR);

    e->base.w = e->track.image.subtex->width;
    e->base.h = e->track.image.subtex->height;
}

UISlider *ui_create_slider(UIScreen *screen) {
    UISlider *e = malloc(sizeof(UISlider));

    if (!e) return NULL;

    memset(e, 0, sizeof(UISlider));
    e->base.type = UI_SLIDER;
    e->base.enabled = true;
    
    ui_element_apply_default_properties(&e->base, screen);

    ui_slider_init_graphics(e);

    e->value = 1;
    e->max_value = 1;

    e->base.update = ui_slider_update;
    e->base.draw = ui_slider_draw;
    e->base.destroy = ui_slider_destroy;

    return e;
}

UIElement *ui_create_slider_from_props(UIScreen *screen, const UIPropertyList *props) {
    UISlider *slider = ui_create_slider(screen);

    if (!slider) return NULL;

    ui_element_apply_properties(&slider->base, screen, props);

    slider->max_value = ui_prop_float(props, "max_value", 100);

    // Please no divisions by zero, thanks
    if (slider->max_value == 0) {
        slider->max_value = 100;
    }
    
    ui_slider_init_graphics(slider);

    return &slider->base;
}