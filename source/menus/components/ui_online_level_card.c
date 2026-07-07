#include "ui_element.h"
#include <citro2d.h>
#include "ui_image.h"
#include "text.h"
#include "fonts/bigFont.h"
#include "fonts/goldFont.h"
#include "ui_screen.h"
#include "easing.h"
#include "math_helpers.h"
#include "menus/components/ui_online_level_card.h"
#include "menus/components/ui_window_button.h"
#include "main.h"
#include "utils/folders.h"

static void ui_online_level_card_update(UIElement* e, UIInput* touch) {
    float right_side = e->x + e->w / 2;
    float button_pos = right_side - 32;

    float width = e->online_level_card.windowbutton_w;
    float height = e->online_level_card.windowbutton_h;

    bool pressedTouch = hidKeysDown() & KEY_TOUCH;
    bool releasedTouch = hidKeysUp() & KEY_TOUCH;

    bool inside = touch->touchPosition.px >= button_pos - (width / 2) && touch->touchPosition.px < button_pos + (width / 2) &&
                  touch->touchPosition.py >= e->y - (height / 2) && touch->touchPosition.py < e->y + (height / 2);

    // Check if pressed the button
    if (inside && pressedTouch && !touch->did_something) {
        e->online_level_card.windowbutton.hovered = true;
        e->online_level_card.windowbutton.pressed = true;
    }

    // If previously pressed on it, hover
    if (inside) {
        e->online_level_card.windowbutton.hovered = true;
    }
    
    // If released on button, do its action
    if (e->online_level_card.windowbutton.hovered && releasedTouch) {
        e->online_level_card.windowbutton.pressed = false;
        e->online_level_card.windowbutton.hovered = false;
        e->online_level_card.windowbutton.hoverTimer = 0.f;
        e->online_level_card.windowbutton.hoverScale = 1.f;
        if (e->action)
            e->action(e);
    }
    
    // Unpress the button
    if (!inside) {
        e->online_level_card.windowbutton.hovered = false;
    }
    
    // Mask background elements
    if (inside) {
        touch->interacted = true;
        touch->did_something = true;
    }
}

static void ui_online_level_card_draw(UIElement* e) {
    float left_side = e->x - e->w / 2;
    float right_side = e->x + e->w / 2;
    float top_side = e->y - e->h / 2;
    float name_pos = left_side + 50;
    float creator_pos = left_side + 55;
    float song_pos = left_side + 52;
    float length_pos = left_side + 62;
    float length_img_pos = left_side + 55;
    float downloads_pos = left_side + 112;
    float downloads_img_pos = left_side + 105;
    float likes_pos = left_side + 162;
    float likes_img_pos = left_side + 155;
    float stars_pos = left_side + 30;
    float stars_img_pos = left_side + 17;
    float difficulty_pos = left_side + 24;
    float button_pos = right_side - 32;
    
    
    C2D_DrawRectSolid(left_side, top_side, 0, e->w, e->h, (e->online_level_card.swap_color ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));

    draw_text(&bigFont_fontCharset, &bigFont_sheet, name_pos, e->y-17, e->online_level_card.name.scale, e->online_level_card.name.scale,0.f, true, "%s", e->online_level_card.name.text);

    draw_text(&goldFont_fontCharset, &goldFont_sheet, creator_pos, e->y-4.5f, e->online_level_card.creator.scale, e->online_level_card.creator.scale,0.f, true, "%s", e->online_level_card.creator.text);

    draw_text(&bigFont_fontCharset, &bigFont_sheet, song_pos, e->y+7, e->online_level_card.song.scale, e->online_level_card.song.scale, 0.f, true, "%s", e->online_level_card.song.text);

    draw_text(&bigFont_fontCharset, &bigFont_sheet, length_pos, e->y+19, e->online_level_card.length.scale, e->online_level_card.length.scale,0.f, true, "%s", e->online_level_card.length.text);

    draw_text(&bigFont_fontCharset, &bigFont_sheet, downloads_pos, e->y+19, 0.35f, 0.35f, 0.f, true, "%d", e->online_level_card.downloads);

    draw_text(&bigFont_fontCharset, &bigFont_sheet, likes_pos, e->y+19, 0.35f, 0.35f, 0.f, true, "%d", e->online_level_card.likes);

    draw_text(&bigFont_fontCharset, &bigFont_sheet, stars_pos, e->y+19, 0.35f, 0.35f, 0.5f, true, "%d", e->online_level_card.stars);


    // draw stars icon
    C2D_SpriteFromSheet(&e->online_level_card.stars_icon.sprite, *get_sheet(0), 170);
    C3D_TexSetFilter(e->online_level_card.stars_icon.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteSetCenter(&e->online_level_card.stars_icon.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->online_level_card.stars_icon.sprite, stars_img_pos, e->y+19);
    C2D_SpriteSetScale(&e->online_level_card.stars_icon.sprite, 0.71f, 0.71f);
    C2D_DrawSprite(&e->online_level_card.stars_icon.sprite);

    // draw difficulty face
    C2D_SpriteFromSheet(&e->online_level_card.difficulty.sprite, *get_sheet(0), 244);
    C3D_TexSetFilter(e->online_level_card.difficulty.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteSetCenter(&e->online_level_card.difficulty.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->online_level_card.difficulty.sprite, difficulty_pos, e->y-4);
    C2D_SpriteSetScale(&e->online_level_card.difficulty.sprite, 0.82f, 0.82f);
    C2D_DrawSprite(&e->online_level_card.difficulty.sprite);

    // draw length icon
    C2D_SpriteFromSheet(&e->online_level_card.length_icon.sprite, *get_sheet(0), 197);
    C3D_TexSetFilter(e->online_level_card.length_icon.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteSetCenter(&e->online_level_card.length_icon.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->online_level_card.length_icon.sprite, length_img_pos, e->y+19);
    C2D_SpriteSetScale(&e->online_level_card.length_icon.sprite, 0.5f, 0.5f);
    C2D_DrawSprite(&e->online_level_card.length_icon.sprite);

    // draw downloads icon
    C2D_SpriteFromSheet(&e->online_level_card.downloads_icon.sprite, *get_sheet(0), 163);
    C3D_TexSetFilter(e->online_level_card.downloads_icon.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteSetCenter(&e->online_level_card.downloads_icon.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->online_level_card.downloads_icon.sprite, downloads_img_pos, e->y+19);
    C2D_SpriteSetScale(&e->online_level_card.downloads_icon.sprite, 0.7f, 0.7f);
    C2D_DrawSprite(&e->online_level_card.downloads_icon.sprite);

    // draw likes icon
    C2D_SpriteFromSheet(&e->online_level_card.likes_icon.sprite, *get_sheet(0), 166);
    C3D_TexSetFilter(e->online_level_card.likes_icon.sprite.image.tex, GPU_LINEAR, GPU_LINEAR);

    C2D_SpriteSetCenter(&e->online_level_card.likes_icon.sprite, 0.5f, 0.5f);
    C2D_SpriteSetPos(&e->online_level_card.likes_icon.sprite, likes_img_pos, e->y+19);
    C2D_SpriteSetScale(&e->online_level_card.likes_icon.sprite, 0.7f, 0.7f);
    C2D_DrawSprite(&e->online_level_card.likes_icon.sprite);

    //draw stupid windowbutton

    EaseTypes bounce_type;

    // Animation
    if (e->online_level_card.windowbutton.hovered)
    {
        e->online_level_card.windowbutton.hoverTimer += DT * (e->online_level_card.windowbutton.keyPressTimer > 0 ? 2 : 1);
        bounce_type = (e->online_level_card.windowbutton.keyPressTimer > 0 ? EASE_OUT : BOUNCE_OUT);
    }
    else
    {
        e->online_level_card.windowbutton.hoverTimer -= DT;
        
        // As the animation plays in reverse, we just use bounce in
        bounce_type = BOUNCE_IN;
    }

    e->online_level_card.windowbutton.hoverTimer = clampf(e->online_level_card.windowbutton.hoverTimer, 0.f, WINDOW_BUTTON_HOVER_ANIM_TIME);
    e->online_level_card.windowbutton.hoverScale = easeValue(bounce_type, 1.0f, WINDOW_BUTTON_HOVER_SCALE, e->online_level_card.windowbutton.hoverTimer, WINDOW_BUTTON_HOVER_ANIM_TIME, 0);
    const LabelFont *font = &fonts[0];

    float scale = e->online_level_card.windowbutton.hoverScale;
    float text_scale;

    draw_9_slice(e->online_level_card.windowbutton.window.atlas, button_pos, e->y, e->online_level_card.windowbutton_w * scale, e->online_level_card.windowbutton_h * scale, e->online_level_card.windowbutton.window.border, e->online_level_card.windowbutton.window.color);
    text_scale = (e->online_level_card.windowbutton.textScale * scale);

    draw_text(font->charset, font->sheet, button_pos, e->y, text_scale, text_scale, 0.5f, true, "%s", "View");
}

UIElement ui_create_online_level_card(int x, int y, bool swap_color, char *name, char *creator, char *song, char *length, int downloads, int likes, int stars, char (*tag)[TAG_LENGTH]) {
    UIElement e = {0};

    e.type = UI_ONLINE_LEVEL_CARD;
    e.x = x;
    e.y = y;
    e.w = 246;
    e.h = 60;
    e.enabled = true;
    
    // Copy tag
    copy_tag_array(&e, tag);

    // Copy text
    truncate_filename(name, 12);
    truncate_filename(creator, 16);
    truncate_filename(song, 22);
    strncpy(e.online_level_card.name.text, name, 15);
    strncpy(e.online_level_card.creator.text, creator, 19);
    strncpy(e.online_level_card.song.text, song, 25);
    strncpy(e.online_level_card.length.text, length, 255);

    e.online_level_card.downloads = downloads;
    e.online_level_card.likes = likes;
    e.online_level_card.stars = stars;
    e.online_level_card.swap_color = swap_color;

    e.online_level_card.name.scale = 0.54f;

    e.online_level_card.creator.scale = 0.45f;

    e.online_level_card.song.scale = 0.35f;

    e.online_level_card.length.scale = 0.35f;

    e.online_level_card.windowbutton_h = 28;
    e.online_level_card.windowbutton_w = 48;

    e.online_level_card.windowbutton.hoverScale = 1.f;
    e.online_level_card.windowbutton.textScale = 0.48f;
    e.online_level_card.windowbutton.window.atlas = C2D_SpriteSheetGetImage(window_sheet, 5);
    e.online_level_card.windowbutton.window.border = e.online_level_card.windowbutton.window.atlas.subtex->width / 3;
    e.online_level_card.windowbutton.window.color = C2D_Color32(255, 255, 255, 255); 


    e.update = ui_online_level_card_update;
    e.draw = ui_online_level_card_draw;

    return e;
}