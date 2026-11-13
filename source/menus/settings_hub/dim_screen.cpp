#include <3ds.h>
#include <citro2d.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_slider.hpp"

#include "menus/settings_hub/dim_screen.hpp"

namespace gd3ds
{

static UISlider *dimSlider;

static void dimScreenInit(UIScreen *s) {
    dimSlider = (UISlider *) uiGetElementByTag(s, "dim_slider");
    uiSliderSetValue(dimSlider, dimSecondaryScreen);
}

static void dimScreenUpdate(UIScreen *s, UIInput *i) {
    if (dimSlider) dimSecondaryScreen = dimSlider->value;
}

extern const UIScreenDefPair dimScreenDef = {
    .name = "dim_screen",
    .btm = {
        .path = "romfs:/menus/settings_hub/dim_screen.txt",
        .init = dimScreenInit,
        .update = dimScreenUpdate
    }
};

}