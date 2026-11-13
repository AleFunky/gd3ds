#pragma once

#include "menus/core/ui_screen.hpp"

namespace gd3ds
{

void uiImageSetTint(UIImage* e, u32 color);
void uiImageClearTint(UIImage* e);
void uiImageSetImage(UIImage *e, int spriteIndex, int sheet);

void uiImageDraw(UIElement* e, UITransform *transform);

UIImage *uiCreateImage(UIScreen *screen);
UIElement *uiCreateImageFromProps(UIScreen *screen, const UIPropertyList *props);

}