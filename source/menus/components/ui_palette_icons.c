#include "icons.h"

#include <citro2d.h>
#include "menus/core/ui_screen.h"
#include "menus/core/ui_props.h"
#include "menus/palette_kit.h"
#include "graphics.h"

#define ICON_WIDTH 30

static void ui_palette_icons_draw(UIElement* e, UITransform *transform) {
    UIPaletteIcons *palette_icons = (UIPaletteIcons *) e;

    bool glow_enabled = (player_glow_enabled || ((p1_color.r | p1_color.g | p1_color.b) == 0));
    
    float length = ((GAMEMODE_COUNT - 1) * (ICON_WIDTH + palette_icons->spacing)) * transform->scaleX;

    float x = transform->x - length * 0.5f;
    
    for (size_t g = 0; g < GAMEMODE_COUNT; g++) {
        float icon_x = x + (ICON_WIDTH + palette_icons->spacing) * transform->scaleX * g;
        spawn_icon_at(
            g, *current_icons[g], glow_enabled, icon_x, transform->y, 0, 0, 0, transform->scaleX,
            C2D_Color32(p1_color.r, p1_color.g, p1_color.b, 255),
            C2D_Color32(p2_color.r, p2_color.g, p2_color.b, 255),
            C2D_Color32(glow_color.r, glow_color.g, glow_color.b, 255)
        );
    }
}

static void ui_palette_icons(UIElement *e) {
    if (e) {
        free(e);
        e = NULL;
    }
}

UIPaletteIcons *ui_create_palette_icons(UIScreen *screen) {
    UIPaletteIcons *e = malloc(sizeof(UIPaletteIcons));

    if (!e) return NULL;

    memset(e, 0, sizeof(UIElement));
    e->base.type = UI_PALETTE_ICONS;
    e->base.enabled = true;

    e->base.draw = ui_palette_icons_draw;
    e->base.destroy = ui_palette_icons;
    
    ui_element_apply_default_properties(&e->base, screen);

    return e;
}

UIElement *ui_create_palette_icons_from_props(UIScreen *screen, const UIPropertyList *props) {
    UIPaletteIcons *palette_icons = ui_create_palette_icons(screen);

    if (!palette_icons) return NULL;

    ui_element_apply_properties(&palette_icons->base, screen, props);

    palette_icons->spacing = ui_prop_float(props, "spacing", 20);

    return &palette_icons->base;
}