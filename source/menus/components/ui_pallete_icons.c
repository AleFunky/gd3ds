#include "ui_element.h"
#include <citro2d.h>
#include "ui_screen.h"
#include "ui_bg_gradient.h"
#include "menus/palette_kit.h"
#include "graphics.h"

static void ui_pallete_icons_update(UIElement* e, UIInput* touch) {
    bool inside = touch->touchPosition.px >= e->x - (e->w / 2) && touch->touchPosition.px < e->x + (e->w / 2) &&
                  touch->touchPosition.py >= e->y - (e->h / 2) && touch->touchPosition.py < e->y + (e->h / 2);
    
    if (inside) touch->did_something = true;
}

static void ui_pallete_icons_draw(UIElement* e) {
    bool glow_enabled = (player_glow_enabled || ((p1_color.r | p1_color.g | p1_color.b) == 0));
    
    for (size_t g = 0; g < GAMEMODE_COUNT; g++) {
        spawn_icon_at(
            g, *current_icons[g], glow_enabled, 90 + g * 35, 70, 0, 0, 0, 0.7f,
            C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
            C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
            C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
        );
    }
}

UIElement ui_create_pallete_icons(char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_PALLETE_ICONS;
    e.x = 0;
    e.y = 0;
    e.enabled = true;

    copy_tag_array(&e, tag);

    e.update = ui_pallete_icons_update;
    e.draw = ui_pallete_icons_draw;

    return e;
}