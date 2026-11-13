#include "icons.hpp"

#include <citro2d.h>
#include "menus/core/ui_screen.hpp"
#include "menus/core/ui_props.hpp"
#include "menus/palette_kit.hpp"
#include "graphics.hpp"
#include "player/robot_anim_data.hpp"
#include "player/spider_anim_data.hpp"

#define ICON_WIDTH 30

static void uiPaletteIconsDraw(UIElement* e, UITransform *transform) {
    UIPaletteIcons *paletteIcons = (UIPaletteIcons *) e;

    bool glowEnabled = (playerGlowEnabled || ((p1Color.r | p1Color.g | p1Color.b) == 0));
    
    float length = ((ICON_GAMEMODE_COUNT - 1) * (ICON_WIDTH + paletteIcons->spacing)) * transform->scaleX;

    float x = transform->x - length * 0.5f;
    
    for (size_t g = 0; g < ICON_GAMEMODE_COUNT; g++) {
        float iconX = x + (ICON_WIDTH + paletteIcons->spacing) * transform->scaleX * g;
        spawnIconAt(
            g, *currentIcons[g], glowEnabled, iconX, transform->y, 0, 0, 0, transform->scaleX,
            C2D_Color32(p1Color.r, p1Color.g, p1Color.b, 255),
            C2D_Color32(p2Color.r, p2Color.g, p2Color.b, 255),
            C2D_Color32(glowColor.r, glowColor.g, glowColor.b, 255),
            (IconParameters) { 
                .animID = (g == (int)GAMEMODE_ROBOT ? (int)ROBOT_ANIM_IDLE : (int)SPIDER_ANIM_IDLE),
                .animFrame = 0
            }
        );
    }
}

static void uiPaletteIcons(UIElement *e) {
    if (e) {
        delete (UIPaletteIcons *)e;
        e = NULL;
    }
}

UIPaletteIcons *uiCreatePaletteIcons(UIScreen *screen) {
    UIPaletteIcons *e = new UIPaletteIcons{};

    if (!e) return NULL;

    e->base.type = UI_PALETTE_ICONS;
    e->base.enabled = true;

    e->base.draw = uiPaletteIconsDraw;
    e->base.destroy = uiPaletteIcons;
    
    uiElementApplyDefaultProperties(&e->base, screen);

    return e;
}

UIElement *uiCreatePaletteIconsFromProps(UIScreen *screen, const UIPropertyList *props) {
    UIPaletteIcons *paletteIcons = uiCreatePaletteIcons(screen);

    if (!paletteIcons) return NULL;

    uiElementApplyProperties(&paletteIcons->base, screen, props);

    paletteIcons->spacing = uiPropFloat(props, "spacing", 20);

    return &paletteIcons->base;
}