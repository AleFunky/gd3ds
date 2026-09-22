#include "ui_stack.h"
#include "main.h"
#include "ui_screen.h"
#include "screen_definitions.h"
#include "mp3_player.h"
#include "fonts/bigFont.h"

static UIStack *stack = NULL;

void ui_stack_set_stack(UIStack *set_stack){
    stack = set_stack;
}

static size_t ui_stack_min_index() {
    return stack->current_root;
}

static size_t ui_stack_max_index() {
    if(stack->active_count == 0){
        return 0;
    }
    return stack->current_root + stack->active_count - 1;
}

static size_t ui_stack_next_index() {
    return stack->current_root + stack->active_count;
}

static void unload_screenpair(int index){
    UIScreenPair *pair = stack->screen_stack[index];

    if(!pair) return;

    if(pair->free_data && pair->data){
        pair->free_data(pair->data);
    }
    pair->free_data = NULL;
    pair->data = NULL;

    for(int j = 0; j < 2; j++){
        UIScreen *screen = &pair->screens[j];
        ui_unload_screen(screen);
    }

    free(pair);
    stack->screen_stack[index] = NULL;
}

static void open_root(){
    //unload all active layers below new root
    for(size_t i = ui_stack_min_index(); i <= ui_stack_max_index(); i++){
        //basic unload of previous screens (do not unload entire screenpair)
        UIScreenPair *pair = stack->screen_stack[i];
        for(int j = 0; j < 2; j++){
            UIScreen *screen = &pair->screens[j];
            ui_unload_screen(screen);
        }
    }

    size_t index = ui_stack_next_index();

    stack->current_root = index;
    stack->active_count = 1;
}

static void close_root(){
    if(stack->current_root == 0) return;

    //unload all active layers above root
    for(size_t i = ui_stack_min_index(); i <= ui_stack_max_index(); i++){
        unload_screenpair(i);
    }

    //find previous root
    size_t new_root = 0;
    for(size_t i = stack->current_root; i-- > 0; ){
        if(stack->screen_stack[i]->root){
            new_root = i;
            break;
        }
    }

    for(size_t i = new_root; i < stack->current_root; i++){
        UIScreenPair *pair = stack->screen_stack[i];
        for(int j = 0; j < 2; j++){
            UIScreen *screen = &pair->screens[j];
            ui_load_screen(screen);
            finish_animation(screen);
        }
    }

    stack->active_count = stack->current_root - new_root;

    stack->current_root = new_root;
}

static bool resize_stack(size_t new_capacity){
    if(new_capacity > 32){
        return false;
    }

    size_t capacity = stack->stack_capacity;

    if(new_capacity <= capacity){
        return true;
    }

    while(capacity < new_capacity){
        capacity += 4;
    }

    UIScreenPair **temp = realloc(stack->screen_stack, sizeof(UIScreenPair *) * capacity);

    if(!temp){
        return false;
    }

    memset(temp + stack->stack_capacity, 0, sizeof(UIScreenPair *) * (capacity - stack->stack_capacity));

    stack->screen_stack = temp;
    stack->stack_capacity = capacity;

    return true;
}

//create new screenpair and expand stack if needed
static UIScreenPair *ui_stack_open(const UIScreenDefPair *defs){
    if(!defs){
        return NULL;
    }

    int index = ui_stack_next_index();

    if(!resize_stack(index + 1)){
        return NULL;
    }

    UIScreenPair *pair = calloc(1, sizeof(UIScreenPair));

    if(!pair) return NULL;

    stack->screen_stack[index] = pair;

    pair->name = defs->name;

    UIScreen *top = &pair->screens[SCREEN_TOP];
    UIScreen *btm = &pair->screens[SCREEN_BTM];

    *top = (UIScreen){ 0 };
    *btm = (UIScreen){ 0 };

    btm->isBottom = true;

    top->def = &defs->top;
    btm->def = &defs->btm;

    top->pair = pair;
    btm->pair = pair;

    return pair;
}

void ui_stack_push(
    const UIScreenDefPair* defs, 
    UIAnimation top_anim, 
    UIAnimation btm_anim,
    UIStackPushType type
){
    //neither of these types actually push a screen
    if(type == PUSH_NONE || type == PUSH_GAME_STATE){
        return;
    }

    // Hi cloud, this seems to be kinda limiting and it works so :vglue:
    //if(stack->root_transition != UI_TRANSITION_NONE){
    //    output_log("Cannot push during transition!\n");
    //    return;
    //}

    if(stack->push.type != PUSH_NONE){
        output_log("Cannot queue multiple pushes!\n");
        return;
    }

    stack->push.defs = defs;
    stack->push.top_anim = top_anim;
    stack->push.btm_anim = btm_anim;
    stack->push.type = type;

    if(type == PUSH_ROOT){
        stack->root_transition = UI_TRANSITION_OPENING;
        stack->fade = FADE_STATUS_IN;
        stack->fade_time = 0.f;
    }
}

void ui_stack_push_name(
    const char* name, 
    UIAnimation top_anim, 
    UIAnimation btm_anim,
    UIStackPushType type
){
    const UIScreenDefPair *defs = ui_get_screen_def(name);

    if(!defs){
        return;
    }

    ui_stack_push(defs, top_anim, btm_anim, type);
}

void ui_stack_push_data(void *data){
    //can't push data when there is no actual push
    if(stack->push.type == PUSH_NONE) return;


    stack->push.data = data;
}

void ui_stack_push_root_instant(const UIScreenDefPair *defs){
    UIScreenPair *pair = ui_stack_open(defs);

    if (!pair) {
        return;
    }

    pair->root = true;

    open_root();

    UIScreen *top = &pair->screens[SCREEN_TOP];
    UIScreen *btm = &pair->screens[SCREEN_BTM];

    ui_load_screen(top);
    ui_load_screen(btm);

    finish_animation(top);
    finish_animation(btm);
}

void ui_stack_push_game_state(int game_state){
    stack->next_game_state = game_state;
    stack->push.type = PUSH_GAME_STATE;
    stack->root_transition = UI_TRANSITION_OPENING;
    stack->fade = FADE_STATUS_IN;
    stack->fade_time = 0.f;
}

void ui_stack_pop(){
    if(stack->root_transition != UI_TRANSITION_NONE) {
        output_log("Cannot pop stack during transition!\n");
        return;
    }

    if(stack->active_count == 0) return;

    UIScreenPair *pair = stack->screen_stack[ui_stack_max_index()];
    if(pair->root){
       stack->root_transition = UI_TRANSITION_CLOSING;
       stack->fade = FADE_STATUS_IN;
       stack->fade_time = 0.f;
    } else{
        for(int i = 0; i < 2; i++){
            ui_screen_close(&pair->screens[i]);
        }
    }
}

//pops all active screens
void ui_stack_pop_context(){
    if(stack->root_transition != UI_TRANSITION_NONE) {
        output_log("Cannot pop stack during transition!\n");
        return;
    }

    for(size_t i = ui_stack_min_index() + 1; i < ui_stack_max_index(); i++){
        for(int j = 0; j < 2; j++){
            UIScreen *screen = &stack->screen_stack[i]->screens[j];
            ui_screen_close(screen);
        }
    }

    stack->root_transition = UI_TRANSITION_CLOSING;
    stack->fade = FADE_STATUS_IN;
    stack->fade_time = 0.f;
}

static void update_opening(){
    UIStackPush *push = &stack->push;

    if(push->type == PUSH_NEXT){
        push->push_now = true;
    }

    //do push
    if(push->push_now && push->type != PUSH_NONE){
        push->push_now = false;

        UIScreenPair *pair = ui_stack_open(push->defs);

        if(!pair){
            push->type = PUSH_NONE;
            return;
        }

        pair->root = (push->type == PUSH_ROOT);
        if(push->defs->free_data){
            pair->free_data = push->defs->free_data;
        }
        if(push->data){
            pair->data = push->data;
        }
        push->data = NULL;

        push->type = PUSH_NONE;

        UIScreen *top = &pair->screens[SCREEN_TOP];
        UIScreen *btm = &pair->screens[SCREEN_BTM];

        if(pair->root){
            open_root();
        } else{
            stack->active_count++;
        }

        ui_load_screen(top);
        ui_load_screen(btm);

        ui_screen_open(top, push->top_anim);
        ui_screen_open(btm, push->btm_anim);
    }
}

static void update_closing(){
    UIScreenPair *pair = stack->screen_stack[ui_stack_max_index()];

    if(!pair) return;

    if(!pair->root){
        UIScreen *top = &pair->screens[SCREEN_TOP];
        UIScreen *btm = &pair->screens[SCREEN_BTM];

        bool top_closing = top->loaded ? top->closing : true;
        bool btm_closing = btm->loaded ? btm->closing : true;

        if(stack->root_transition == UI_TRANSITION_NONE && top_closing && btm_closing){
            unload_screenpair(ui_stack_max_index());

            stack->active_count--;

            if(stack->push.type == PUSH_AFTER_CLOSE){
                stack->push.push_now = true;
            }
        }
    }
}

static void handle_stack_fading(){
    //root transition open/close slop
    //instant if the active count is 0 (main menu doesn't fade in)
    bool transition_switch = false;
    if(stack->fade_time >= 255.f || stack->active_count == 0){
        switch(stack->fade){
            case FADE_STATUS_IN:
                stack->fade_time = 0.f;
                stack->fade = FADE_STATUS_OUT;
                transition_switch = true;
                break;
            case FADE_STATUS_OUT:
                stack->fade_time = 0.f;
                stack->fade = FADE_STATUS_NONE;
                stack->root_transition = UI_TRANSITION_NONE;
                break;
            case FADE_STATUS_NONE:
                break;
        }
    }

    //when the fade is fully black, either go back to the previous root or open a new one depending on transition
    if(transition_switch) {
        if(stack->push.type == PUSH_GAME_STATE){
            game_state = stack->next_game_state;
            escape_state = true;
            stack->fade_time = 0.f;
            stack->fade = FADE_STATUS_OUT;
            stack->push.type = PUSH_NONE;
        } else{
            switch(stack->root_transition){
                case UI_TRANSITION_OPENING:
                    if(stack->push.type == PUSH_ROOT) stack->push.push_now = true;
                    stack->fade_time = 0.f;
                    stack->fade = FADE_STATUS_OUT;
                    break;
                case UI_TRANSITION_CLOSING:
                    close_root();
                    stack->fade_time = 0.f;
                    stack->fade = FADE_STATUS_OUT;
                    break;
                case UI_TRANSITION_NONE:
                    break;
            }
        }
        fixed_dt = true;
    } else{
        //make sure there's a frame of full black
        stack->fade_time += FADE_SPEED * DT;
    }
}

void ui_stack_update(UIInput *input){
    handle_stack_fading();

    update_opening();

    if(stack->active_count > 0){
        for(size_t i = ui_stack_min_index(); i <= ui_stack_max_index(); i++){
            UIScreenPair *pair = stack->screen_stack[i];

            if(!pair) continue;

            bool updating_topmost = i == ui_stack_max_index() && stack->root_transition == UI_TRANSITION_NONE;
        
            for(int j = 0; j < 2; j++) {
                UIScreen *screen = &pair->screens[j];

                UIInput dummy_input = { 0 };
                //only update top screen with input
                UIInput *input_to_use = updating_topmost && screen->transition.done ? input : &dummy_input;

                ui_screen_update(screen, input_to_use);
            }
        }

        update_closing();
    }
}

void draw_stack_debug(){
    char debug[2048];
    size_t pos = 0;

    for (size_t i = 0; i <= ui_stack_max_index(); i++) {
        UIScreenPair *pair = stack->screen_stack[i];

        if(!pair) continue;

        int r = 0;
        if(pair->root){
            r = 128;
        }
        if(i == stack->current_root){
            r = 255;
        }
        int gb = 128;
        if(i > ui_stack_min_index() && i <= ui_stack_max_index()){
            gb = 255;
        }

        pos += snprintf(
            debug + pos,
            sizeof(debug) - pos,
            "<%d,%d,%d>%s</>\n",
            r, gb, gb,
            pair->name ? pair->name : "(unnamed)"
        );

        if (pos >= sizeof(debug)) break;
    }

    char rootSlop[] = "ROOT:....";
    snprintf(rootSlop, sizeof(rootSlop), "ROOT: %d", stack->current_root);

    char activeSlop[] = "Active:....";
    snprintf(activeSlop, sizeof(activeSlop), "Active: %d", stack->active_count);

    char capacitySlop[] = "Capacity:....";
    snprintf(capacitySlop, sizeof(capacitySlop), "Capacity: %d", stack->stack_capacity);

    draw_text(&bigFont_fontCharset, &bigFont_sheet, SCREEN_BOT_WIDTH / 2, SCREEN_HEIGHT / 2, 0.5f, 0.5f, 0, true, debug);
    draw_text(&bigFont_fontCharset, &bigFont_sheet, 10, 10, 0.5f, 0.5f, 0, true, rootSlop);
    draw_text(&bigFont_fontCharset, &bigFont_sheet, 10, 25, 0.5f, 0.5f, 0, true, activeSlop);
    draw_text(&bigFont_fontCharset, &bigFont_sheet, 10, 40, 0.5f, 0.5f, 0, true, capacitySlop);
}

void draw_stack_fade(){
    float fade = 0;
    if(stack->fade == FADE_STATUS_IN){
        fade = stack->fade_time;
    } else if(stack->fade == FADE_STATUS_OUT){
        fade = 255 - stack->fade_time;
    }

    C2D_DrawRectSolid(0.f, 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, fade));
}

void ui_stack_draw(Screens target){
    if(stack->active_count == 0) return;

    for(size_t i = ui_stack_min_index(); i <= ui_stack_max_index(); i++){
        UIScreenPair *pair = stack->screen_stack[i];

        if(!pair) continue;

        UIScreen *screen = &pair->screens[target];
        if(screen->loaded){
            ui_screen_draw(screen);
        }
    }
}

void ui_stack_clear(){
    for(size_t i = 0; i < stack->stack_capacity; i++){
        unload_screenpair(i);
    }

    free(stack->screen_stack);
    *stack = (UIStack){ 0 };
}

UIScreen *ui_stack_get_screen(const char *name, Screens screen) {
    UIScreenPair *pair = NULL;

    for(size_t i = ui_stack_min_index(); i <= ui_stack_max_index(); i++){
        UIScreenPair *pair1 = stack->screen_stack[i];

        if(!pair1) continue;

        if(strcmp(pair1->name, name) == 0){
            pair = pair1;
        }
    }

    if(!pair) return NULL;
    
    return &pair->screens[screen];
}