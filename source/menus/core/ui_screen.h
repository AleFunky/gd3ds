#pragma once
#include "menus/core/ui_element.h"
#include "text.h"
#include "common_setters.h"

#define MAX_ELEMENT_PROPERTIES 64

typedef struct UIScreenPair UIScreenPair;

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

typedef enum {
    ANIM_NONE,
    ANIM_ZOOM,
    ANIM_ZOOM_SUBTLE,
    ANIM_BOUNCE_DOWN,
    ANIM_BOUNCE_DOWN_SLOW,
    ANIM_SLIDE_RIGHT,
    ANIM_SLIDE_DOWN,

    NUM_OPEN_ANIMS
} UIAnimation;

typedef enum {
    UI_TRANSITION_NONE,
    UI_TRANSITION_OPENING,
    UI_TRANSITION_CLOSING
} UITransitionState;

typedef enum {
    UI_DRAW_BEFORE,
    UI_DRAW_AFTER
} UIDrawPhase;

typedef struct {
    UIAnimation animation;
    UITransitionState state;

    float time;
    float duration;
    //determined based on the animation specified (only really applies to the slide down anim for out_duration = 0.5)
    float in_duration;
    float out_duration;
    //amount of time it takes the (fullscreen) darken to fade in/out relative to the screen opening animation
    float darken_frac;

    bool done;
} UITransition;

typedef struct {
    const char* path;

    void (*init)(UIScreen *);
    void (*update)(UIScreen *, UIInput *);
    void (*draw)(UIScreen *, UIDrawPhase);
    void (*exit)(UIScreen *);

    UIActionList action_list;
} UIScreenDefinition;

typedef struct {
    const char *name;
    UIScreenDefinition top;
    UIScreenDefinition btm;
    void (*free_data)(void *data);
} UIScreenDefPair;

typedef struct UIScreen {
    UIScreenPair *pair;
    const UIScreenDefinition *def;

    UIElement **elements;
    size_t count;
    size_t capacity;

    UITransition transition;
    bool isBottom:1;
    bool disable_element_update:1;

    bool loaded:1;
    bool closing:1;
} UIScreen;

typedef void (*UIElementVisitor)(UIElement *element, void *userdata);
typedef bool (*UIElementPredicate)(UIElement *, void *);
typedef UIElement *(*UICreateFn)(UIScreen *screen, const UIPropertyList *);

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

extern const UIBitfieldEntry keybind_table[22];

void required_loading_screen_assets_init();
void ui_assets_init();

C2D_SpriteSheet *get_sheet(int sheet);

void copy_tag_array(UIElement *e, const char *tags);

void finish_animation(UIScreen *screen);

void ui_screen_open(UIScreen *screen, UIAnimation animation);
void ui_screen_close(UIScreen *screen);

UIElement *ui_get_element_by_tag(UIScreen *screen, const char *tag);
void ui_run_func_on_tag(UIScreen *screen, const char *tag, void (*func)(UIElement *e));

void ui_set_pos_on_tag(UIScreen *screen, float x, float y, const char *tag);

char* next_token(char** cursor);
void collect_properties(UIPropertyList *props, char *token, char **cursor);

void add_ui_particle_system(ParticleSystem *particle);
void free_ui_particle_systems();

void ui_load_screen(UIScreen* screen);
void ui_load_screen_old(UIScreen* screen, const UIActionDef* actions, size_t action_count, const char* path);
void ui_screen_update_transition(UIScreen *screen, float dt);
void ui_screen_update(UIScreen* screen, UIInput* touch);
void ui_screen_draw(UIScreen* screen);
void ui_unload_screen(UIScreen *screen);