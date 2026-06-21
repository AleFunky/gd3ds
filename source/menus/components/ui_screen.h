#pragma once
#include "ui_element.h"
#include "text.h"

#define UI_MAX_ELEMENTS 128
#define UI_MAX_PARTICLE_SYSTEMS 32

typedef enum {
    ANIM_NONE,
    ANIM_ZOOM,
    ANIM_SLIDE_RIGHT,

    NUM_OPEN_ANIMS
} ScreenOpenAnim;

typedef struct {
    UIElement elements[UI_MAX_ELEMENTS];
    int count;
    float open_anim_time;
    bool open_anim_done;
    ScreenOpenAnim open_anim;
    bool isBottom;
    bool disable_element_update;
} UIScreen;

typedef struct {
    const char* name;
    UIActionFn fn;
} UIAction;

typedef struct {
    const Charset *charset;
    C2D_SpriteSheet *sheet;    
} LabelFont;

enum Fonts {
    FONT_PUSAB,
    FONT_CHAT,
    FONT_GOLD_PUSAB,

    NUM_FONTS
};

extern C2D_SpriteSheet ui_sheet;
extern C2D_SpriteSheet ui_2_sheet;
extern C2D_SpriteSheet bigFont_sheet;
extern C2D_SpriteSheet chatFont_sheet;
extern C2D_SpriteSheet goldFont_sheet;
extern C2D_SpriteSheet window_sheet;
extern C2D_SpriteSheet bg_gradient_sheet;
extern C2D_SpriteSheet bar_sheet;

extern UIScreen default_screen;
extern UIScreen default_screen_top;

extern const LabelFont fonts[NUM_FONTS];

void required_loading_screen_assets_init();
void ui_assets_init();

C2D_SpriteSheet *get_sheet(int sheet);

void copy_tag_array(UIElement *e, char (*tag)[TAG_LENGTH]);
void ui_load_screen(UIScreen* screen, const UIAction* actions, size_t count, const char* path);
void ui_unload_screen(UIScreen *screen);

void run_animation_slide(UIScreen *screen, bool go_up);

void ui_screen_update(UIScreen* screen, UIInput* touch);
void ui_screen_draw(UIScreen* screen);
UIElement *ui_get_element_by_tag(UIScreen *screen, const char *tag);
void ui_run_func_on_tag(UIScreen *screen, const char *tag, void (*func)(UIElement *e));

void ui_set_pos_on_tag(UIScreen *screen, float x, float y, const char *tag);

// Premade functions for on "ui_run_func_on_tag"

void ui_enable_element(UIElement *e);
void ui_disable_element(UIElement *e);

void add_ui_particle_system(ParticleSystem *particle);
void free_ui_particle_systems();