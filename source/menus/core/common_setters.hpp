#pragma once
#include "ui_element.hpp"

namespace gd3ds
{

void uiElementSetPosition(UIElement *e, float x, float y);
void uiElementSetSize(UIElement *e, int w, int h);
void uiElementSetEnabled(UIElement *e, bool enabled);

void uiElementSetScale(UIElement *e, float scale);
void uiElementSetScaleX(UIElement *e, float scale);
void uiElementSetScaleY(UIElement *e, float scale);
void uiElementSetScaleXy(UIElement *e, float x, float y);

void uiElementSetAction(UIElement *e, UIActionFn action);

}