#include "menus/core/ui_screen.hpp"
#include "main.hpp"
#include "menus/components/ui_spinner.hpp"
#include "menus/core/common_setters.hpp"
#include "menus/settings_hub/settings.hpp"
#include "ui_element.hpp"
#include "ui_screen.hpp"

#include "menus/components/ui_button.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_checkbox.hpp"
#include "menus/components/ui_window.hpp"
#include "menus/components/ui_textbox.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_bg_gradient.hpp"
#include "menus/components/ui_darken.hpp"
#include "menus/components/ui_icon.hpp"
#include "menus/components/ui_color_button.hpp"
#include "menus/components/ui_window_button.hpp"
#include "menus/components/ui_progress_bar.hpp"
#include "menus/components/ui_particle.hpp"
#include "menus/components/ui_use_effect.hpp"
#include "menus/components/ui_slider.hpp"
#include "menus/components/ui_palette_icons.hpp"
#include "menus/components/ui_rectangle.hpp"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>

#include "graphics.hpp"

#include "fonts/bigFont.hpp"
#include "fonts/chatFont.hpp"
#include "fonts/goldFont.hpp"
#include "utils/gfx.hpp"
#include "utils/string_helpers.hpp"

C2D_SpriteSheet uiSheet;
C2D_SpriteSheet ui2Sheet;
C2D_SpriteSheet windowSheet;
C2D_SpriteSheet bigFontSheet;
C2D_SpriteSheet chatFontSheet;
C2D_SpriteSheet goldFontSheet;
C2D_SpriteSheet bgGradientSheet;
C2D_SpriteSheet barSheet;

UIScreen defaultScreen = {
    .isBottom = true
};
UIScreen defaultScreenTop;

typedef struct {
    const char *name;
    UICreateFn create;
} UIElementConstructor;

static const UIElementConstructor elementConstructors[] = {
    {"button",       uiCreateButtonFromProps },
    {"image",        uiCreateImageFromProps },
    {"label",        uiCreateLabelFromProps },
    {"checkbox",     uiCreateCheckboxFromProps },
    {"window",       uiCreateWindowFromProps },
    {"textbox",      uiCreateTextboxFromProps },
    {"list",         uiCreateListFromProps },
    {"bggradient",   uiCreateBgGradientFromProps },
    {"darken",       uiCreateDarkenFromProps },
    {"icon",         uiCreateIconFromProps },
    {"colorbutton",  uiCreateColorButtonFromProps },
    {"windowbutton", uiCreateWindowButtonFromProps },
    {"progressbar",  uiCreateProgressBarFromProps },
    {"particle",     uiCreateParticleFromProps },
    {"useeffect",    uiCreateUseEffectFromProps },
    {"paletteicons", uiCreatePaletteIconsFromProps },
    {"slider",       uiCreateSliderFromProps },
    {"rectangle",    uiCreateRectangleFromProps },
    {"spinner",      uiCreateSpinnerFromProps },
};

extern const UIBitfieldEntry keybindTable[] = {
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

extern const LabelFont fonts[NUM_FONTS] = {
    {
        .charset = &bigFontFontCharset,
        .sheet = &bigFontSheet
    },
    {
        .charset = &chatFontFontCharset,
        .sheet = &chatFontSheet
    },
    {
        .charset = &goldFontFontCharset,
        .sheet = &goldFontSheet
    }
};

void requiredLoadingScreenAssetsInit() {
    uiSheet = C2D_SpriteSheetLoad("romfs:/gfx/ui.t3x");
    if (!uiSheet) svcBreak(USERBREAK_PANIC);

    ui2Sheet = C2D_SpriteSheetLoad("romfs:/gfx/ui_2.t3x");
    if (!ui2Sheet) svcBreak(USERBREAK_PANIC);

    goldFontSheet = C2D_SpriteSheetLoad("romfs:/gfx/goldFont.t3x");
    if (!goldFontSheet) svcBreak(USERBREAK_PANIC);

    barSheet = C2D_SpriteSheetLoad("romfs:/gfx/bars.t3x");
    if (!barSheet) svcBreak(USERBREAK_PANIC);

    bgSheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_sheet_01.t3x");
    if (!bgSheet) svcBreak(USERBREAK_PANIC);
}

void uiAssetsInit() {
    windowSheet = C2D_SpriteSheetLoad("romfs:/gfx/windows.t3x");
    if (!windowSheet) svcBreak(USERBREAK_PANIC);

    bigFontSheet = C2D_SpriteSheetLoad("romfs:/gfx/bigFont.t3x");
    if (!bigFontSheet) svcBreak(USERBREAK_PANIC);

    chatFontSheet = C2D_SpriteSheetLoad("romfs:/gfx/chatFont.t3x");
    if (!chatFontSheet) svcBreak(USERBREAK_PANIC);

    bgGradientSheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_gradient.t3x");
    if (!bgGradientSheet) svcBreak(USERBREAK_PANIC);
    
    groundSheet = C2D_SpriteSheetLoad("romfs:/gfx/grounds.t3x");
    if (!groundSheet) svcBreak(USERBREAK_PANIC);
}

C2D_SpriteSheet getSheet(int sheet) {
    switch (sheet) {
        case 0:
            return uiSheet;
        case 1:
            return ui2Sheet;
        case 2:
            return windowSheet;
        case 3:
            return spriteSheet;
        case 4:
            return spriteSheet2;
        case 5:
            return bgSheet;
        case 6:
            return groundSheet;
        case 9:
            return barSheet;
        case 10:
            return bgGradientSheet;
    }
    return NULL;
}

// Starts the opening animation
void uiScreenOpen(UIScreen *screen, UIAnimation animation) {
    if (!screen)
        return;

    if(animation == ANIM_NONE){
        finishAnimation(screen);
    }

    UITransition *t = &screen->transition;

    t->animation = animation;

    //could probably use a table for this stuff but whatevs

    t->inDuration = 0.f;
    t->outDuration = 0.f;
    t->darkenFrac = 1.f;

    if(animation != ANIM_NONE) t->inDuration = 0.5f;

    switch (animation) {
    case ANIM_SLIDE_DOWN:
        t->outDuration = 0.5f;
        break;

    case ANIM_BOUNCE_DOWN:
        t->outDuration = 0.3f;
        t->darkenFrac = 0.6f;
        break;

    case ANIM_BOUNCE_DOWN_SLOW:
        t->inDuration = 1;
        t->outDuration = 0.6f;
        t->darkenFrac = 0.6f;
        break;

    default:
        t->darkenFrac = 0.3f;
        break;
    };

    t->state = UI_TRANSITION_OPENING;

    if(t->inDuration <= 0.f){
        t->time = t->duration;
        t->done = true;
    }

    t->duration = t->inDuration;
    t->time = 0.0f;
    t->done = false;
}

// Starts the opening animation in reverse
void uiScreenClose(UIScreen *screen) {
    if (!screen)
        return;

    UITransition *t = &screen->transition;

    if (settingsState.fastMenu) {
        t->state = UI_TRANSITION_CLOSING;
        t->time = t->duration;
        t->done = true;
        screen->closing = true;
        return;
    }

    t->state = UI_TRANSITION_CLOSING;

    if(t->outDuration <= 0.f){
        t->time = t->duration;
        t->done = true;
        screen->closing = true;
    }

    t->duration = t->outDuration;
    t->time = 0.0f;
    t->done = false;
}

void uiScreenUpdateTransition(UIScreen *screen, float dt) {
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
void uiScreenUpdate(UIScreen* s, UIInput* touch) {
    uiScreenUpdateTransition(s, DT);

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

    for (int i = (int)s->count - 1; i >= 0; i--) {
        UIElement *e = s->elements[i];
        uiUpdateTree(e, touch, &identity);
    }
}

static void uiScreenHandleAnim(UIScreen* s, UITransform *root) {
    int width = s->isBottom ? 320 : 400;
    int height = 240;

    float cx = width * 0.5f;
    float cy = height * 0.5f;

    float slideValue = 0;
    float scaleValue = 0;

    // Animation stuff
    switch (s->transition.animation) {
        case ANIM_ZOOM:
            scaleValue = easeValue(ELASTIC_OUT, 0.f, 1.f, s->transition.time, s->transition.duration, 0.6f);
            
            if (s->transition.state == UI_TRANSITION_CLOSING) scaleValue = 1.f - scaleValue;

            root->scaleX = scaleValue;
            root->scaleY = scaleValue;

            root->x = cx * (1.f - scaleValue);
            root->y = cy * (1.f - scaleValue);
            break;
        case ANIM_ZOOM_SUBTLE:
            scaleValue = easeValue(ELASTIC_OUT, 0.f, 1.f, s->transition.time, s->transition.duration / 1.5f, 1.6f);
            
            if (s->transition.state == UI_TRANSITION_CLOSING) scaleValue = 1.f - scaleValue;

            root->scaleX = scaleValue;
            root->scaleY = scaleValue;

            root->x = cx * (1.f - scaleValue);
            root->y = cy * (1.f - scaleValue);
            break;
        case ANIM_BOUNCE_DOWN:
        case ANIM_BOUNCE_DOWN_SLOW:
            if (s->transition.state == UI_TRANSITION_CLOSING) 
            slideValue = 1.f - easeValue(EASE_IN, 0.f, 1.f, s->transition.time, s->transition.duration, 2.f); 
            else 
            slideValue = easeValue(BOUNCE_OUT, 0.f, 1.f, s->transition.time, s->transition.duration, 1.f);
            

            root->y = -(1.f - slideValue) * height;
            break;
        case ANIM_SLIDE_RIGHT:
            slideValue = easeValue(ELASTIC_OUT, 0.f, 1.f, s->transition.time, s->transition.duration, 0.6f);

            if (s->transition.state == UI_TRANSITION_CLOSING) slideValue = 1.f - slideValue;

            root->x = -(1.f - slideValue) * (width / 2.f);
            break;
        case ANIM_SLIDE_DOWN:
            slideValue = easeValue(EASE_IN_OUT, 0.f, 1.f, s->transition.time, s->transition.duration, 2.f);

            if (s->transition.state == UI_TRANSITION_CLOSING) slideValue = 1.f - slideValue;

            root->y = -(1.f - slideValue) * height;
            break;
        default:
            break;
    }
}

// Draw all screen characters
void uiScreenDraw(UIScreen* s) {
    if (!s->loaded || s->closing) return;

    bool customDraw = s->def && s->def->draw;

    if(customDraw) s->def->draw(s, UI_DRAW_BEFORE);

    UITransform root = {
        .x = 0.f,
        .y = 0.f,
        .scaleX = 1.f,
        .scaleY = 1.f
    };

    uiScreenHandleAnim(s, &root);

    for (size_t i = 0; i < s->count; i++) {
        UIElement *e = s->elements[i];

        uiDrawTree(e, &root);
    }

    if(customDraw) s->def->draw(s, UI_DRAW_AFTER);
}

void finishAnimation(UIScreen *screen) {
    // Surely no animation lasts more than 31 years
    screen->transition.time = 999999999.f;
    screen->transition.done = true;
}

// This gets a line and converts it into a null terminated string
static void trimNewline(char* s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

static void convertNewLine(char *str) {
    for (char *p = str; *p; p++) {
        if (*p == '\\' && p[1] == 'n') {
            *p = '\n';
            memmove(p + 1, p + 2, strlen(p + 2) + 1);
        }
    }
}

// Searches for the next token
char* nextToken(char** cursor) {
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
UIElement *uiFindElement(UIElement *element, UIElementPredicate predicate, void *userdata) {
    if (!element) return NULL;

    // Check this element
    if (predicate(element, userdata))
        return element;

    // Check children
    for (UIElement *child = element->firstChild; child; child = child->nextSibling) {
        UIElement *found = uiFindElement(child, predicate, userdata);

        if (found) return found;
    }

    // Nothing in this element
    return NULL;
}

// Generic element visitor
void uiVisitTree(UIElement *element, UIElementVisitor visitor, void *userdata) {
    if (!element) return;

    visitor(element, userdata);

    for (UIElement *child = element->firstChild; child; child = child->nextSibling) {
        uiVisitTree(child, visitor, userdata);
    }
}

static bool hasTag(UIElement *element, void *userdata) {
    const char *tag = (char *)userdata;

    for (int i = 0; i < TAGS_PER_ELEMENT; i++) {
        if (strcmp(element->tag[i], tag) == 0)
            return true;
    }

    return false;
}

// Get element by its tag, returns NULL if there is no elements with that tag
UIElement *uiGetElementByTag(UIScreen *screen, const char *tag) {
    for (size_t i = 0; i < screen->count; i++) {
        UIElement *found = uiFindElement(screen->elements[i], hasTag, (void *)tag);

        if (found) return found;
    }

    return NULL;
}

typedef struct {
    const char *tag;
    void (*func)(UIElement *);
} UITagVisitorData;

static void visitTag(UIElement *element, void *userdata) {
    UITagVisitorData *data = (UITagVisitorData *)userdata;

    for (int i = 0; i < TAGS_PER_ELEMENT; i++) {
        if (strcmp(element->tag[i], data->tag) == 0) {
            data->func(element);
            break;
        }
    }
}

// Run a function on each element with an specific tag
void uiRunFuncOnTag(UIScreen *screen, const char *tag, void (*func)(UIElement *e)) {
    UITagVisitorData data = { .tag = tag, .func = func };

    for (size_t i = 0; i < screen->count; i++) {
        uiVisitTree(screen->elements[i], visitTag, &data);
    }
}

// DEPRECATED: use child elements and just move the parent
void uiSetPosOnTag(UIScreen *screen, float x, float y, const char *tag) {
    bool foundParent = false;
    float movementX = 0;
    float movementY = 0;
    for (size_t i = 0; i < screen->count; i++) {
        for (int j = 0; j < TAGS_PER_ELEMENT; j++) {
            UIElement *e = screen->elements[i];
            // Check for element with this tag
            if (strcmp(e->tag[j], tag) == 0) {
                if (!foundParent) {
                    foundParent = true;
                    movementX = x - e->x;
                    movementY = y - e->y;
                }

                e->x += movementX;
                e->y += movementY;
            }
        }
    }
}

static void splitTags(const char *input, char tag[][TAG_LENGTH]) {
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

void copyTagArray(UIElement *e, const char *tags) {
    if (tags == NULL) return;

    char tag[TAGS_PER_ELEMENT][TAG_LENGTH] = { 0 };

    // Split the tags
    splitTags(tags, tag);

    memcpy(e->tag, tag, sizeof(tag));
}

UNUSED static void printProps(UIPropertyList *props) {
    outputLog("START\n");
    for (size_t i = 0; i < props->count; i++) {
        outputLog("KEY %s VALUE %s\n",props->properties[i].key, props->properties[i].value);
    }
    outputLog("END\n");
}

void uiScreenAddElement(UIScreen *screen, UIElement *element) {
    // Expand if out of space
    if (screen->count == screen->capacity) {
        screen->capacity *= 2;

        screen->elements = (UIElement **)realloc(
            screen->elements,
            screen->capacity * sizeof(*screen->elements)
        );
    }

    screen->elements[screen->count++] = element;
    element->screen = screen;
}

void collectProperties(UIPropertyList *props, char *token, char **cursor){
    while ((token = nextToken(cursor)) != NULL) {
        char* equal = strchr(token, '=');
        if (!equal) continue;

        // This replaces the equal sign between key and value with a null character, dividing the string in two
        *equal = '\0';

        char* key = token;
        char* value = equal + 1;

        stripEnclosures(value);

        convertNewLine(value);

        uiProplistAdd(props, key, value);
    }
}

#define MAX_NESTED_CHILDREN 32

//UIScreenDefinition for the screen is already set at this point
void uiLoadScreen(UIScreen* screen) {
    if (!screen || !screen->def || !screen->def->path) {
        return;
    }

    // Unload screen if already loaded
    if (screen->loaded) {
        uiUnloadScreen(screen);
    }

    FILE* f = fopen(screen->def->path, "r");
    if (!f) return;

    screen->loaded = true;

    screen->disableElementUpdate = false;
    screen->transition.time = 0.f;
    screen->transition.done = false;

    screen->count = 0;
    screen->capacity = 16;
    screen->elements = (UIElement **)calloc(screen->capacity, sizeof(*screen->elements));

    char line[512];

    UIElement *childStack[MAX_NESTED_CHILDREN];

    int stackPtr = 0;

    UIElement *last_element = NULL;

    // Iterate through lines (one element per line)
    while (fgets(line, sizeof(line), f)) {
        trimNewline(line);

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
            if (stackPtr < MAX_NESTED_CHILDREN) {
                childStack[stackPtr++] = last_element;
            }
        } else if (p[0] == '}') {
            if (stackPtr > 0) {
                stackPtr--;
            }
        }
        
        char* cursor = line;
        char* token = nextToken(&cursor);

        // Check for invalid tokens
        if (!token) continue;

        // The element type
        char type[16];
        strncpy(type, token, 15);

        UIPropertyList props = uiCreateProplist(MAX_ELEMENT_PROPERTIES, false);

        // Parse element parameters
        collectProperties(&props, token, &cursor);

        // Execute the element constructor
        for (size_t i = 0; i < ARRAY_LEN(elementConstructors); i++) {
            if (strcmp(type, elementConstructors[i].name) == 0) {
                if (elementConstructors[i].create) {
                    UIElement *e = elementConstructors[i].create(screen, &props);
                    //print_props(&props);

                    if (stackPtr > 0) {
                        uiElementAddChild(childStack[stackPtr - 1], e);
                    } else {
                        uiScreenAddElement(screen, e);
                    }

                    last_element = e;
                }
            }
        }

        uiDestroyProplist(&props);
    }
    
    fclose(f);

    if(screen->def && screen->def->init){
        screen->def->init(screen);
    }
}

void uiUnloadScreen(UIScreen *screen) {
    if (!screen->loaded || !screen->elements) return;

    if(screen->def && screen->def->exit){
        screen->def->exit(screen);
    }

    for (size_t i = 0; i < screen->count; i++) {
        UIElement *e = screen->elements[i];
        uiDestroyTree(e);
    }

    free(screen->elements);
    screen->elements = NULL;
    screen->closing = false;
    screen->loaded = false;
}

//please remember to remove eventually
const UIScreenDefinition dummyDefTemp = {
    0
};

// Load a screen from its file, needs a pointer to the actions table and the action count
void uiLoadScreenOld(UIScreen* screen, const UIActionDef* actions, size_t actionCount, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    
    // Unload screen if already loaded
    if (screen->loaded) {
        uiUnloadScreen(screen);
    }

    screen->loaded = true;

    if(!screen->def) screen->def = &dummyDefTemp;

    screen->disableElementUpdate = false;
    screen->transition.time = 0.f;
    screen->transition.done = false;

    screen->count = 0;
    screen->capacity = 16;
    screen->elements = (UIElement **)calloc(screen->capacity, sizeof(*screen->elements));

    char line[512];

    UIElement *childStack[MAX_NESTED_CHILDREN];

    int stackPtr = 0;

    UIElement *last_element = NULL;

    // Iterate through lines (one element per line)
    while (fgets(line, sizeof(line), f)) {
        trimNewline(line);

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
            if (stackPtr < MAX_NESTED_CHILDREN) {
                childStack[stackPtr++] = last_element;
            }
        } else if (p[0] == '}') {
            if (stackPtr > 0) {
                stackPtr--;
            }
        }
        
        char* cursor = line;
        char* token = nextToken(&cursor);

        // Check for invalid tokens
        if (!token) continue;

        // The element type
        char type[16];
        strncpy(type, token, 15);

        UIPropertyList props = uiCreateProplist(MAX_ELEMENT_PROPERTIES, false);

        // Parse element parameters
        collectProperties(&props, token, &cursor);

        // Execute the element constructor
        for (size_t i = 0; i < ARRAY_LEN(elementConstructors); i++) {
            if (strcmp(type, elementConstructors[i].name) == 0) {
                if (elementConstructors[i].create) {
                    UIElement *e = elementConstructors[i].create(screen, &props);
                    //print_props(&props);

                    if (stackPtr > 0) {
                        uiElementAddChild(childStack[stackPtr - 1], e);
                    } else {
                        uiScreenAddElement(screen, e);
                    }

                    last_element = e;
                }
            }
        }

        uiDestroyProplist(&props);
    }
    
    fclose(f);
}