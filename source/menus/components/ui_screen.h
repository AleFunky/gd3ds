#pragma once
#include "ui_element.h"

#define UI_MAX_ELEMENTS 128

typedef struct {
    UIElement elements[UI_MAX_ELEMENTS];
    int count;
} UIScreen;

typedef struct {
    const char* name;
    UIActionFn fn;
} UIAction;

extern C2D_SpriteSheet ui_sheet;
extern C2D_SpriteSheet ui_2_sheet;
extern C2D_SpriteSheet bigFont_sheet;
extern C2D_SpriteSheet chatFont_sheet;
extern C2D_SpriteSheet window_sheet;
extern C2D_SpriteSheet bg_gradient_sheet;
extern C2D_SpriteSheet bar_sheet;

void ui_assets_init();

C2D_SpriteSheet *get_sheet(int sheet);

void copy_tag_array(UIElement *e, char (*tag)[TAG_LENGTH]);
void ui_load_screen(UIScreen* screen, const UIAction* actions, size_t count, const char* path);

void ui_screen_update(UIScreen* screen, UIInput* touch);
void ui_screen_draw(UIScreen* screen);
UIElement *ui_get_element_by_tag(UIScreen *screen, const char *tag);
void ui_run_func_on_tag(UIScreen *screen, const char *tag, void (*func)(UIElement *e));

void ui_set_pos_on_tag(UIScreen *screen, float x, float y, const char *tag);

// Premade functions for on "ui_run_func_on_tag"

void ui_enable_element(UIElement *e);
void ui_disable_element(UIElement *e);