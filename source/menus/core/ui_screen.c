#include "menus/core/ui_screen.h"
#include "main.h"
#include "menus/components/ui_spinner.h"
#include "menus/core/common_setters.h"
#include "ui_element.h"
#include "ui_screen.h"

#include "menus/components/ui_button.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window.h"
#include "menus/components/ui_textbox.h"
#include "menus/components/ui_list.h"
#include "menus/components/ui_bg_gradient.h"
#include "menus/components/ui_darken.h"
#include "menus/components/ui_icon.h"
#include "menus/components/ui_color_button.h"
#include "menus/components/ui_window_button.h"
#include "menus/components/ui_progress_bar.h"
#include "menus/components/ui_particle.h"
#include "menus/components/ui_use_effect.h"
#include "menus/components/ui_slider.h"
#include "menus/components/ui_palette_icons.h"
#include "menus/components/ui_rectangle.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>

#include "graphics.h"

#include "fonts/bigFont.h"
#include "fonts/chatFont.h"
#include "fonts/goldFont.h"
#include "utils/gfx.h"
#include "utils/string_helpers.h"

C2D_SpriteSheet ui_sheet;
C2D_SpriteSheet ui_2_sheet;
C2D_SpriteSheet window_sheet;
C2D_SpriteSheet bigFont_sheet;
C2D_SpriteSheet chatFont_sheet;
C2D_SpriteSheet goldFont_sheet;
C2D_SpriteSheet bg_gradient_sheet;
C2D_SpriteSheet bar_sheet;

UIScreen default_screen = {
    .isBottom = true
};
UIScreen default_screen_top;

typedef struct {
    const char *name;
    UICreateFn create;
} UIElementConstructor;

static const UIElementConstructor element_constructors[] = {
    {"button",       ui_create_button_from_props },
    {"image",        ui_create_image_from_props },
    {"label",        ui_create_label_from_props },
    {"checkbox",     ui_create_checkbox_from_props },
    {"window",       ui_create_window_from_props },
    {"textbox",      ui_create_textbox_from_props },
    {"list",         ui_create_list_from_props },
    {"bggradient",   ui_create_bg_gradient_from_props },
    {"darken",       ui_create_darken_from_props },
    {"icon",         ui_create_icon_from_props },
    {"colorbutton",  ui_create_color_button_from_props },
    {"windowbutton", ui_create_window_button_from_props },
    {"progressbar",  ui_create_progress_bar_from_props },
    {"particle",     ui_create_particle_from_props },
    {"useeffect",    ui_create_use_effect_from_props },
    {"paletteicons", ui_create_palette_icons_from_props },
    {"slider",       ui_create_slider_from_props },
    {"rectangle",    ui_create_rectangle_from_props },
    {"spinner",      ui_create_spinner_from_props },
};

const UIBitfieldEntry keybind_table[] = {
    { "A",      KEY_A },
    { "B",      KEY_B },
    { "X",      KEY_X },
    { "Y",      KEY_Y },
    { "L",      KEY_L },
    { "R",      KEY_R },
    { "ZL",     KEY_ZL },
    { "ZR",     KEY_ZR },

    { "UP",     KEY_DUP },
    { "DOWN",   KEY_DDOWN },
    { "LEFT",   KEY_DLEFT },
    { "RIGHT",  KEY_DRIGHT },

    { "START",  KEY_START },
    { "SELECT", KEY_SELECT },

    { "CSUP",    KEY_CSTICK_UP },
    { "CSDOWN",  KEY_CSTICK_DOWN },
    { "CSLEFT",  KEY_CSTICK_LEFT },
    { "CSRIGHT", KEY_CSTICK_RIGHT },

    { "CPUP",    KEY_CPAD_UP },
    { "CPDOWN",  KEY_CPAD_DOWN },
    { "CPLEFT",  KEY_CPAD_LEFT },
    { "CPRIGHT", KEY_CPAD_RIGHT },
};

const LabelFont fonts[NUM_FONTS] = {
    {
        .charset = &bigFont_fontCharset,
        .sheet = &bigFont_sheet
    },
    {
        .charset = &chatFont_fontCharset,
        .sheet = &chatFont_sheet
    },
    {
        .charset = &goldFont_fontCharset,
        .sheet = &goldFont_sheet
    }
};

void required_loading_screen_assets_init() {
    ui_sheet = C2D_SpriteSheetLoad("romfs:/gfx/ui.t3x");
    if (!ui_sheet) svcBreak(USERBREAK_PANIC);

    ui_2_sheet = C2D_SpriteSheetLoad("romfs:/gfx/ui_2.t3x");
    if (!ui_2_sheet) svcBreak(USERBREAK_PANIC);

    goldFont_sheet = C2D_SpriteSheetLoad("romfs:/gfx/goldFont.t3x");
    if (!goldFont_sheet) svcBreak(USERBREAK_PANIC);

    bar_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bars.t3x");
    if (!bar_sheet) svcBreak(USERBREAK_PANIC);

    bgSheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_sheet_01.t3x");
    if (!bgSheet) svcBreak(USERBREAK_PANIC);
}

void ui_assets_init() {
    window_sheet = C2D_SpriteSheetLoad("romfs:/gfx/windows.t3x");
    if (!window_sheet) svcBreak(USERBREAK_PANIC);

    bigFont_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bigFont.t3x");
    if (!bigFont_sheet) svcBreak(USERBREAK_PANIC);

    chatFont_sheet = C2D_SpriteSheetLoad("romfs:/gfx/chatFont.t3x");
    if (!chatFont_sheet) svcBreak(USERBREAK_PANIC);

    bg_gradient_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_gradient.t3x");
    if (!bg_gradient_sheet) svcBreak(USERBREAK_PANIC);
    
    bg2Sheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_sheet_02.t3x");
    if (!bg2Sheet) svcBreak(USERBREAK_PANIC);
    
    groundSheet = C2D_SpriteSheetLoad("romfs:/gfx/grounds.t3x");
    if (!groundSheet) svcBreak(USERBREAK_PANIC);
}

C2D_SpriteSheet *get_sheet(int sheet) {
    switch (sheet) {
        case 0:
            return &ui_sheet;
        case 1:
            return &ui_2_sheet;
        case 2:
            return &window_sheet;
        case 3:
            return &spriteSheet;
        case 4:
            return &spriteSheet2;
        case 5:
            return &bgSheet;
        case 6:
            return &groundSheet;
        case 7:
            return &cube0Sheet;
        case 8:
            return &bg2Sheet;
        case 9:
            return &bar_sheet;
        case 10:
            return &bg_gradient_sheet;
        case 11: 
            return &cube1Sheet;
        case 12:
            return &shipSheet;
        case 13:
            return &ballSheet;
        case 14:
            return &ufoSheet;
        case 15:
            return &waveSheet;
    }
    return NULL;
}

// Starts the opening animation
void ui_screen_open(UIScreen *screen, UIAnimation animation) {
    if (!screen)
        return;

    if(animation == ANIM_NONE){
        finish_animation(screen);
    }

    UITransition *t = &screen->transition;

    t->animation = animation;

    //could probably use a table for this stuff but whatevs

    t->in_duration = 0.f;
    t->out_duration = 0.f;
    t->darken_frac = 1.f;

    if(animation != ANIM_NONE) t->in_duration = 0.5f;

    switch (animation) {
    case ANIM_SLIDE_DOWN:
        t->out_duration = 0.5f;
        break;

    case ANIM_BOUNCE_DOWN:
        t->out_duration = 0.3f;
        t->darken_frac = 0.6f;
        break;

    case ANIM_BOUNCE_DOWN_SLOW:
        t->in_duration = 1;
        t->out_duration = 0.6f;
        t->darken_frac = 0.6f;
        break;

    default:
        t->darken_frac = 0.3f;
        break;
    };

    t->state = UI_TRANSITION_OPENING;

    if(t->in_duration <= 0.f){
        t->time = t->duration;
        t->done = true;
    }

    t->duration = t->in_duration;
    t->time = 0.0f;
    t->done = false;
}

// Starts the opening animation in reverse
void ui_screen_close(UIScreen *screen) {
    if (!screen)
        return;

    UITransition *t = &screen->transition;

    t->state = UI_TRANSITION_CLOSING;

    if(t->out_duration <= 0.f){
        t->time = t->duration;
        t->done = true;
        screen->closing = true;
    }

    t->duration = t->out_duration;
    t->time = 0.0f;
    t->done = false;
}

void ui_screen_update_transition(UIScreen *screen, float dt) {
    if (!screen)
        return;

    UITransition *t = &screen->transition;

    if (t->done || t->state == UI_TRANSITION_NONE)
        return;

    t->time += dt;

    if (t->time >= t->duration) {
        t->time = t->duration;
        t->done = true;

        if (t->state == UI_TRANSITION_CLOSING) {
            screen->closing = true;
        }
    }
}

// Update all screen characters
void ui_screen_update(UIScreen* s, UIInput* touch) {
    ui_screen_update_transition(s, DT);

    if (!s || !s->loaded || s->closing) return;

    if(s->def && s->def->update){
        s->def->update(s, touch);
    }

    UITransform identity = {
        .x = 0.f,
        .y = 0.f,
        .scaleX = 1.f,
        .scaleY = 1.f
    };

    for (int i = s->count - 1; i >= 0; i--) {
        UIElement *e = s->elements[i];
        ui_update_tree(e, touch, &identity);
    }
}

static void ui_screen_handle_anim(UIScreen* s, UITransform *root) {
    int width = s->isBottom ? 320 : 400;
    int height = 240;

    float cx = width * 0.5f;
    float cy = height * 0.5f;

    float slide_value = 0;
    float scale_value = 0;

    // Animation stuff
    switch (s->transition.animation) {
        case ANIM_ZOOM:
            scale_value = easeValue(ELASTIC_OUT, 0.f, 1.f, s->transition.time, s->transition.duration, 0.6f);
            
            if (s->transition.state == UI_TRANSITION_CLOSING) scale_value = 1.f - scale_value;

            root->scaleX = scale_value;
            root->scaleY = scale_value;

            root->x = cx * (1.f - scale_value);
            root->y = cy * (1.f - scale_value);
            break;
        case ANIM_ZOOM_SUBTLE:
            scale_value = easeValue(ELASTIC_OUT, 0.f, 1.f, s->transition.time, s->transition.duration / 1.5f, 1.6f);
            
            if (s->transition.state == UI_TRANSITION_CLOSING) scale_value = 1.f - scale_value;

            root->scaleX = scale_value;
            root->scaleY = scale_value;

            root->x = cx * (1.f - scale_value);
            root->y = cy * (1.f - scale_value);
            break;
        case ANIM_BOUNCE_DOWN:
        case ANIM_BOUNCE_DOWN_SLOW:
            if (s->transition.state == UI_TRANSITION_CLOSING) 
            slide_value = 1.f - easeValue(EASE_IN, 0.f, 1.f, s->transition.time, s->transition.duration, 2.f); 
            else 
            slide_value = easeValue(BOUNCE_OUT, 0.f, 1.f, s->transition.time, s->transition.duration, 1.f);
            

            root->y = -(1.f - slide_value) * height;
            break;
        case ANIM_SLIDE_RIGHT:
            slide_value = easeValue(ELASTIC_OUT, 0.f, 1.f, s->transition.time, s->transition.duration, 0.6f);

            if (s->transition.state == UI_TRANSITION_CLOSING) slide_value = 1.f - slide_value;

            root->x = -(1.f - slide_value) * (width / 2.f);
            break;
        case ANIM_SLIDE_DOWN:
            slide_value = easeValue(EASE_IN_OUT, 0.f, 1.f, s->transition.time, s->transition.duration, 2.f);

            if (s->transition.state == UI_TRANSITION_CLOSING) slide_value = 1.f - slide_value;

            root->y = -(1.f - slide_value) * height;
            break;
        default:
            break;
    }
}

// Draw all screen characters
void ui_screen_draw(UIScreen* s) {
    if (!s->loaded || s->closing) return;

    bool customDraw = s->def && s->def->draw;

    if(customDraw) s->def->draw(s, UI_DRAW_BEFORE);

    UITransform root = {
        .x = 0.f,
        .y = 0.f,
        .scaleX = 1.f,
        .scaleY = 1.f
    };

    ui_screen_handle_anim(s, &root);

    for (int i = 0; i < s->count; i++) {
        UIElement *e = s->elements[i];

        ui_draw_tree(e, &root);
    }

    if(customDraw) s->def->draw(s, UI_DRAW_AFTER);
}

void finish_animation(UIScreen *screen) {
    // Surely no animation lasts more than 31 years
    screen->transition.time = 999999999.f;
    screen->transition.done = true;
}

// This gets a line and converts it into a null terminated string
static void trim_newline(char* s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

static void convert_new_line(char *str) {
    for (char *p = str; *p; p++) {
        if (*p == '\\' && p[1] == 'n') {
            *p = '\n';
            memmove(p + 1, p + 2, strlen(p + 2) + 1);
        }
    }
}

// Searches for the next token
char* next_token(char** cursor) {
    if (!*cursor) return NULL;

    char* s = *cursor;

    // Skip leading spaces
    while (*s == ' ') s++;

    // If empty string, exit
    if (*s == '\0') {
        *cursor = NULL;
        return NULL;
    }

    char* start = s;
    bool inQuotes = false;
    int inBrackets = 0;

    // Search for quotes or brackets
    while (*s) {
        if (*s == '['){
            inBrackets++;
        } else if (*s == ']'){
            inBrackets--;
        } else if (*s == '"' && !inBrackets) {
            inQuotes = !inQuotes;
        }
        // If not in quotes or brackets and found delimiter, no more iterating
        else if ((*s == ' ' || *s == '\n' || *s == '\r') && !(inQuotes || inBrackets)) {
            break;
        }
        s++;
    }

    // Set null character
    if (*s) {
        *s = '\0';
        *cursor = s + 1;
    } else {
        *cursor = NULL;
    }

    return start;
}

// Generic element search
UIElement *ui_find_element(UIElement *element, UIElementPredicate predicate, void *userdata) {
    if (!element) return NULL;

    // Check this element
    if (predicate(element, userdata))
        return element;

    // Check children
    for (UIElement *child = element->first_child; child; child = child->next_sibling) {
        UIElement *found = ui_find_element(child, predicate, userdata);

        if (found) return found;
    }

    // Nothing in this element
    return NULL;
}

// Generic element visitor
void ui_visit_tree(UIElement *element, UIElementVisitor visitor, void *userdata) {
    if (!element) return;

    visitor(element, userdata);

    for (UIElement *child = element->first_child; child; child = child->next_sibling) {
        ui_visit_tree(child, visitor, userdata);
    }
}

static bool has_tag(UIElement *element, void *userdata) {
    const char *tag = userdata;

    for (int i = 0; i < TAGS_PER_ELEMENT; i++) {
        if (strcmp(element->tag[i], tag) == 0)
            return true;
    }

    return false;
}

// Get element by its tag, returns NULL if there is no elements with that tag
UIElement *ui_get_element_by_tag(UIScreen *screen, const char *tag) {
    for (int i = 0; i < screen->count; i++) {
        UIElement *found = ui_find_element(screen->elements[i], has_tag, (void *)tag);

        if (found) return found;
    }

    return NULL;
}

typedef struct {
    const char *tag;
    void (*func)(UIElement *);
} UITagVisitorData;

static void visit_tag(UIElement *element, void *userdata) {
    UITagVisitorData *data = userdata;

    for (int i = 0; i < TAGS_PER_ELEMENT; i++) {
        if (strcmp(element->tag[i], data->tag) == 0) {
            data->func(element);
            break;
        }
    }
}

// Run a function on each element with an specific tag
void ui_run_func_on_tag(UIScreen *screen, const char *tag, void (*func)(UIElement *e)) {
    UITagVisitorData data = { .tag = tag, .func = func };

    for (int i = 0; i < screen->count; i++) {
        ui_visit_tree(screen->elements[i], visit_tag, &data);
    }
}

// DEPRECATED: use child elements and just move the parent
void ui_set_pos_on_tag(UIScreen *screen, float x, float y, const char *tag) {
    bool found_parent = false;
    float movement_x = 0;
    float movement_y = 0;
    for (int i = 0; i < screen->count; i++) {
        for (int j = 0; j < TAGS_PER_ELEMENT; j++) {
            UIElement *e = screen->elements[i];
            // Check for element with this tag
            if (strcmp(e->tag[j], tag) == 0) {
                if (!found_parent) {
                    found_parent = true;
                    movement_x = x - e->x;
                    movement_y = y - e->y;
                }

                e->x += movement_x;
                e->y += movement_y;
            }
        }
    }
}

static void split_tags(const char *input, char tag[][TAG_LENGTH]) {
    int i = 0;
    char *token = strtok((char *) input, ",");

    while (token != NULL && i < TAGS_PER_ELEMENT)
    {
        strncpy(tag[i], token, TAG_LENGTH - 1);
        tag[i][TAG_LENGTH - 1] = '\0';   // Ensure null terminator
        i++;

        token = strtok(NULL, ",");
    }
}

void copy_tag_array(UIElement *e, const char *tags) {
    if (tags == NULL) return;

    char tag[TAGS_PER_ELEMENT][TAG_LENGTH] = { 0 };

    // Split the tags
    split_tags(tags, tag);

    memcpy(e->tag, tag, sizeof(tag));
}

UNUSED static void print_props(UIPropertyList *props) {
    output_log("START\n");
    for (int i = 0; i < props->count; i++) {
        output_log("KEY %s VALUE %s\n",props->properties[i].key, props->properties[i].value);
    }
    output_log("END\n");
}

void ui_screen_add_element(UIScreen *screen, UIElement *element) {
    // Expand if out of space
    if (screen->count == screen->capacity) {
        screen->capacity *= 2;

        screen->elements = realloc(
            screen->elements,
            screen->capacity * sizeof(*screen->elements)
        );
    }

    screen->elements[screen->count++] = element;
    element->screen = screen;
}

void collect_properties(UIPropertyList *props, char *token, char **cursor){
    while ((token = next_token(cursor)) != NULL) {
        char* equal = strchr(token, '=');
        if (!equal) continue;

        // This replaces the equal sign between key and value with a null character, dividing the string in two
        *equal = '\0';

        char* key = token;
        char* value = equal + 1;

        strip_enclosures(value);

        convert_new_line(value);

        ui_proplist_add(props, key, value);
    }
}

#define MAX_NESTED_CHILDREN 32

//UIScreenDefinition for the screen is already set at this point
void ui_load_screen(UIScreen* screen) {
    if (!screen || !screen->def || !screen->def->path) {
        return;
    }

    // Unload screen if already loaded
    if (screen->loaded) {
        ui_unload_screen(screen);
    }

    FILE* f = fopen(screen->def->path, "r");
    if (!f) return;

    screen->loaded = true;

    screen->disable_element_update = false;
    screen->transition.time = 0.f;
    screen->transition.done = false;

    screen->count = 0;
    screen->capacity = 16;
    screen->elements = calloc(screen->capacity, sizeof(*screen->elements));

    char line[512];

    UIElement *child_stack[MAX_NESTED_CHILDREN];

    int stack_ptr = 0;

    UIElement *last_element = NULL;

    // Iterate through lines (one element per line)
    while (fgets(line, sizeof(line), f)) {
        trim_newline(line);

        char *p = line;

        // Skip leading spaces
        while (isspace((unsigned char) *p)) {
            p++;
        }
        
        // Comment or empty
        if (p[0] == '#' || p[0] == '\0')
            continue;

        // Add last element to the stack
        if (p[0] == '{') {
            if (stack_ptr < MAX_NESTED_CHILDREN) {
                child_stack[stack_ptr++] = last_element;
            }
        } else if (p[0] == '}') {
            if (stack_ptr > 0) {
                stack_ptr--;
            }
        }
        
        char* cursor = line;
        char* token = next_token(&cursor);

        // Check for invalid tokens
        if (!token) continue;

        // The element type
        char type[16];
        strncpy(type, token, 15);

        UIPropertyList props = ui_create_proplist(MAX_ELEMENT_PROPERTIES, false);

        // Parse element parameters
        collect_properties(&props, token, &cursor);

        // Execute the element constructor
        for (int i = 0; i < ARRAY_LEN(element_constructors); i++) {
            if (strcmp(type, element_constructors[i].name) == 0) {
                if (element_constructors[i].create) {
                    UIElement *e = element_constructors[i].create(screen, &props);
                    //print_props(&props);

                    if (stack_ptr > 0) {
                        ui_element_add_child(child_stack[stack_ptr - 1], e);
                    } else {
                        ui_screen_add_element(screen, e);
                    }

                    last_element = e;
                }
            }
        }

        ui_destroy_proplist(&props);
    }
    
    fclose(f);

    if(screen->def && screen->def->init){
        screen->def->init(screen);
    }
}

void ui_unload_screen(UIScreen *screen) {
    if (!screen->loaded || !screen->elements) return;

    if(screen->def && screen->def->exit){
        screen->def->exit(screen);
    }

    for (int i = 0; i < screen->count; i++) {
        UIElement *e = screen->elements[i];
        ui_destroy_tree(e);
    }

    free(screen->elements);
    screen->elements = NULL;
    screen->closing = false;
    screen->loaded = false;
}

//please remember to remove eventually
const UIScreenDefinition dummy_def_temp = {
    0
};

// Load a screen from its file, needs a pointer to the actions table and the action count
void ui_load_screen_old(UIScreen* screen, const UIActionDef* actions, size_t action_count, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    
    // Unload screen if already loaded
    if (screen->loaded) {
        ui_unload_screen(screen);
    }

    screen->loaded = true;

    if(!screen->def) screen->def = &dummy_def_temp;

    screen->disable_element_update = false;
    screen->transition.time = 0.f;
    screen->transition.done = false;

    screen->count = 0;
    screen->capacity = 16;
    screen->elements = calloc(screen->capacity, sizeof(*screen->elements));

    char line[512];

    UIElement *child_stack[MAX_NESTED_CHILDREN];

    int stack_ptr = 0;

    UIElement *last_element = NULL;

    // Iterate through lines (one element per line)
    while (fgets(line, sizeof(line), f)) {
        trim_newline(line);

        char *p = line;

        // Skip leading spaces
        while (isspace((unsigned char) *p)) {
            p++;
        }
        
        // Comment or empty
        if (p[0] == '#' || p[0] == '\0')
            continue;

        // Add last element to the stack
        if (p[0] == '{') {
            if (stack_ptr < MAX_NESTED_CHILDREN) {
                child_stack[stack_ptr++] = last_element;
            }
        } else if (p[0] == '}') {
            if (stack_ptr > 0) {
                stack_ptr--;
            }
        }
        
        char* cursor = line;
        char* token = next_token(&cursor);

        // Check for invalid tokens
        if (!token) continue;

        // The element type
        char type[16];
        strncpy(type, token, 15);

        UIPropertyList props = ui_create_proplist(MAX_ELEMENT_PROPERTIES, false);

        // Parse element parameters
        collect_properties(&props, token, &cursor);

        // Execute the element constructor
        for (int i = 0; i < ARRAY_LEN(element_constructors); i++) {
            if (strcmp(type, element_constructors[i].name) == 0) {
                if (element_constructors[i].create) {
                    UIElement *e = element_constructors[i].create(screen, &props);
                    //print_props(&props);

                    if (stack_ptr > 0) {
                        ui_element_add_child(child_stack[stack_ptr - 1], e);
                    } else {
                        ui_screen_add_element(screen, e);
                    }

                    last_element = e;
                }
            }
        }

        ui_destroy_proplist(&props);
    }
    
    fclose(f);
}