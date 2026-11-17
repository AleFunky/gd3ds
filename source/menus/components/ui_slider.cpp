#include <citro2d.h>
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "utils/gfx.hpp"
#include "ui_slider.hpp"

namespace gd3ds
{

// If currently using an slider
bool sliding;

static void drawButton(UISlider *e, UITransform *transform) {
    float width = e->base.w * transform->scaleX;
    float leftSide = transform->x - (width / 2);
    
    float percent = uiSliderGetPercent(e);
    int buttonX = leftSide + (int)(percent * width);

    C2D_SpriteSetCenter(&e->button, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->button, buttonX, transform->y);
    C2D_SpriteSetScale(&e->button, transform->scaleX, transform->scaleY);
    C2D_DrawSprite(&e->button);
}

static void drawFrame(UISlider *e, UITransform *transform) {
    C2D_SpriteSetCenter(&e->trackFrame, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->trackFrame, transform->x, transform->y);
    C2D_SpriteSetScale(&e->trackFrame, transform->scaleX, transform->scaleY);
    C2D_DrawSprite(&e->trackFrame);
}

static void drawBar(UISlider *e, UITransform *transform) {
    float barWidth = e->track.image.subtex->width;
    
    int pixels = (e->value / e->maxValue) * barWidth;

    if (pixels > 0) {
        if (pixels > barWidth) {
            pixels = barWidth;
        }

        C2D_Sprite spr;
        Tex3DS_SubTexture sub;
        C2D_Image img;
        C2D_ImageTint tint;
        
        float x = transform->x - (e->base.w / 2.f) * transform->scaleX;
        
        C2D_PlainImageTint(&tint, C2D_Color32(50, 190, 240, 255), 1.f);

        sub = selectBox(&e->track.image, 0, 0, pixels, e->track.image.subtex->height);
        img = e->track.image; img.subtex = &sub;

        C2D_SpriteFromSheet(&e->button, barSheet, 4 + e->dragging);
        C3D_TexSetFilter(e->button.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteFromImage(&spr, img);
        C2D_SpriteSetCenter(&spr, 0.f, 0.5f);
        C2D_SpriteSetPos(&spr, x, transform->y);
        C2D_SpriteSetScale(&spr, transform->scaleX, transform->scaleY);
        C2D_DrawSpriteTinted(&spr, &tint);
    }
}

static float sliderButtonX(UISlider* e, UITransform *transform) {
    float width = e->base.w * transform->scaleX;
    float leftSide = transform->x - width / 2;

    float t = e->value / e->maxValue;

    return leftSide + (t * width);
}

static void sliderSetFromX(UISlider* e, UITransform *transform, float px) {
    float width = e->base.w * transform->scaleX;
    float leftSide = transform->x - width / 2;

    float t = (float)(px - leftSide) / width;

    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;

    e->value = t * e->maxValue;
}

static bool sliderTouchingButton(UISlider* e, UIInput* touch, UITransform *transform) {
    int buttonX = sliderButtonX(e, transform);

    float width = e->button.image.subtex->width * transform->scaleX;
    float height = e->button.image.subtex->height * transform->scaleY;

    return touch->touchPos.px >= buttonX - width / 2 &&
           touch->touchPos.px <= buttonX + width / 2 &&
           touch->touchPos.py >= transform->y - height / 2 &&
           touch->touchPos.py <= transform->y + height / 2;
}

void uiSliderSetValue(UISlider *e, float value) {
    if (!e) return;

    e->value = value;
}

float uiSliderGetPercent(UISlider* e) {
    if (!e) return 0;

    return e->value / e->maxValue;
}

static void uiSliderUpdate(UIElement* e, UIInput* touch, UITransform *transform) {
    UISlider* s = (UISlider *) e;

    bool inside = sliderTouchingButton(s, touch, transform);

    bool pressedTouch = touch->down & KEY_TOUCH;
    bool heldTouch = touch->held & KEY_TOUCH;
    bool releasedTouch = touch->up & KEY_TOUCH;

    if (pressedTouch && inside) {
        s->dragging = true;
        sliding = true;
    }

    if (s->dragging && heldTouch) {
        sliderSetFromX(s, transform, touch->touchPos.px);
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

static void uiSliderDraw(UIElement* e, UITransform *transform) {
    UISlider* s = (UISlider *) e;

    // Track
    drawBar(s, transform);
    drawFrame(s, transform);

    // Button
    drawButton(s, transform);
}

static void uiSliderDestroy(UIElement *e) {
    if (e) {
        delete (UISlider *)e;
        e = NULL;
    }
}

static void uiSliderInitGraphics(UISlider *e) {
    C2D_SpriteFromSheet(&e->track, barSheet, 3);
    C3D_TexSetFilter(e->track.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteFromSheet(&e->trackFrame, barSheet, 1);
    C3D_TexSetFilter(e->trackFrame.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteFromSheet(&e->button, barSheet, 4);
    C3D_TexSetFilter(e->button.image.tex, GPU_LINEAR, GPU_LINEAR);

    e->base.w = e->track.image.subtex->width;
    e->base.h = e->track.image.subtex->height;
}

UISlider *uiCreateSlider(UIScreen *screen) {
    UISlider *e = new UISlider{};

    if (!e) return NULL;

    e->base.type = UIElementType::UiSlider;
    e->base.enabled = true;
    
    uiElementApplyDefaultProperties(&e->base, screen);

    uiSliderInitGraphics(e);

    e->value = 1;
    e->maxValue = 1;

    e->base.update = uiSliderUpdate;
    e->base.draw = uiSliderDraw;
    e->base.destroy = uiSliderDestroy;

    return e;
}

UIElement *uiCreateSliderFromProps(UIScreen *screen, const UIPropertyList *props) {
    UISlider *slider = uiCreateSlider(screen);

    if (!slider) return NULL;

    uiElementApplyProperties(&slider->base, screen, props);

    slider->maxValue = uiPropFloat(props, "max_value", 100);

    // Please no divisions by zero, thanks
    if (slider->maxValue == 0) {
        slider->maxValue = 100;
    }
    
    uiSliderInitGraphics(slider);

    return &slider->base;
}

}