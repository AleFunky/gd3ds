#pragma once
#include "ui_screen.h"
#include "graphics.h"

typedef struct UIScreenPair {
    const char* name;
    //top and bottom screen
    UIScreen screens[2];  
    //whether this ScreenPair will be treated as the backmost ScreenPair of the current UI view (menus such as the main menu, level select, etc; NOT popup ScreenPairs like settings or color select)
    bool root;

    void *data;
    void (*free_data)(void *data);
} UIScreenPair;

typedef enum {
    PUSH_NONE,
    PUSH_NEXT,
    PUSH_AFTER_CLOSE,
    PUSH_ROOT,
    PUSH_GAME_STATE
} UIStackPushType;

typedef struct {
    const UIScreenDefPair *defs;
    UIAnimation top_anim;
    UIAnimation btm_anim;
    UIStackPushType type;
    bool push_now;
    void *data;
} UIStackPush;

typedef struct {
    //array of pointers to UIScreenPairs
    UIScreenPair **screen_stack;
    size_t stack_capacity;

    //"root" ScreenPairs represent the lowest layer of the current UI context, such as the main menu, creator menu, or level select (as opposed to popups like settings, the color select, etc)
    //ScreenPairs which are below the current root will be unloaded and not updated; reloaded when the current root/context is exited
    size_t current_root;
    //amount of ScreenPairs to update (starts at root and goes up)
    size_t active_count;

    //used for fade to/from black when changing roots
    UITransitionState root_transition; //open or close
    FadeStatus fade;
    float fade_time;

    UIStackPush push;

    int next_game_state;
} UIStack;

bool ui_stack_check_loaded_root(const UIScreenDefPair *screen);
bool ui_stack_check_loaded_screen_in_root(const UIScreenDefPair *screen);
UIScreenPair *ui_stack_get_loaded_screen(const UIScreenDefPair *screen);

void ui_stack_set_stack(UIStack *set_stack);

void ui_stack_push(const UIScreenDefPair *defs, UIAnimation top_anim, UIAnimation btm_anim, UIStackPushType type);
void ui_stack_push_name(const char *name, UIAnimation top_anim, UIAnimation btm_anim, UIStackPushType type);
void ui_stack_push_data(void *data);
void ui_stack_push_root_instant(const UIScreenDefPair *defs);
void ui_stack_push_game_state(int game_state);

void ui_stack_pop();
void ui_stack_pop_context();

void ui_stack_update(UIInput *input);

void draw_stack_debug();
void draw_stack_fade();
void ui_stack_draw(Screens target);

void ui_stack_clear();

UIScreen *ui_stack_get_screen(const char *name, Screens screen);
UIScreen *ui_stack_get_screen_from_pair(UIScreenPair *pair, Screens screen);
UIScreen *ui_stack_get_screen_relative(UIScreen *screen, Screens which);