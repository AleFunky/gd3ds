#include <3ds.h>
#include <citro2d.h>

#include "main.h"
#include "menus/core/ui_screen.h"
#include "menus/components/ui_slider.h"

#include "menus/settings_hub/dim_screen.h"

static UISlider *dim_slider;

static void dim_screen_init(UIScreen *s) {
    dim_slider = (UISlider *) ui_get_element_by_tag(s, "dim_slider");
    ui_slider_set_value(dim_slider, dim_secondary_screen);
}

static void dim_screen_update(UIScreen *s, UIInput *i) {
    if (dim_slider) dim_secondary_screen = dim_slider->value;
}

const UIScreenDefPair dim_screen_def = {
    .name = "dim_screen",
    .btm = {
        .path = "romfs:/menus/settings_hub/dim_screen.txt",
        .init = dim_screen_init,
        .update = dim_screen_update
    }
};
