#pragma once

#include "menus/core/ui_screen.hpp"

#define BUTTON_HOVER_SCALE 1.25f
#define BUTTON_HOVER_ANIM_TIME 0.4f

void uiButtonDrawText(UIElement *e, UITransform *transform);
void uiButtonUpdate(UIElement* e, UIInput* touch, UITransform *transform);

void uiButtonSetImage(UIButton *e, int spriteIndex, int sheet);
void uiButtonSetText(UIButton *e, const char *text);

void uiButtonModifyTransform(UIElement *e, UITransform *t);

UIButton *uiCreateButton(UIScreen *screen);
UIElement *uiCreateButtonFromProps(UIScreen *screen, const UIPropertyList *props);