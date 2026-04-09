#include "ui_screen.h"
#include "ui_element.h"
#include "ui_button.h"
#include "ui_image.h"
#include "ui_label.h"
#include "ui_label_aller.h"
#include "ui_screen.h"
#include "ui_checkbox.h"
#include "ui_window.h"
#include "ui_textbox.h"
#include "ui_list.h"
#include "ui_bg_gradient.h"
#include "ui_action_area.h"
#include "ui_darken.h"
#include "ui_icon.h"
#include "ui_color_button.h"
#include "ui_window_button.h"
#include "ui_progress_bar.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <citro2d.h>

#include "graphics.h"

C2D_SpriteSheet ui_sheet;
C2D_SpriteSheet ui_2_sheet;
C2D_SpriteSheet window_sheet;
C2D_SpriteSheet bigFont_sheet;
C2D_SpriteSheet chatFont_sheet;
C2D_SpriteSheet bg_gradient_sheet;
C2D_SpriteSheet bar_sheet;

void ui_assets_init() {
    ui_sheet = C2D_SpriteSheetLoad("romfs:/gfx/ui.t3x");
    ui_2_sheet = C2D_SpriteSheetLoad("romfs:/gfx/ui_2.t3x");
    window_sheet = C2D_SpriteSheetLoad("romfs:/gfx/windows.t3x");
    bigFont_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bigFont.t3x");
    chatFont_sheet = C2D_SpriteSheetLoad("romfs:/gfx/chatFont.t3x");
    bg_gradient_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bg_gradient.t3x");
    bar_sheet = C2D_SpriteSheetLoad("romfs:/gfx/bars.t3x");
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
            return &iconSheet;
        case 8:
            return &bg2Sheet;
        case 9:
            return &bar_sheet;
    }
    return NULL;
}

// Update all screen characters
void ui_screen_update(UIScreen* s, UIInput* touch) {
    for (int i = s->count - 1; i >= 0; i--) {
        UIElement *e = &s->elements[i];
        if (e->enabled) e->update(e, touch);
    }
}

// Draw all screen characters
void ui_screen_draw(UIScreen* s) {
    for (int i = 0; i < s->count; i++) {
        UIElement *e = &s->elements[i];
        if (e->enabled) e->draw(e);
    }
}

// Find an action by its name
UIActionFn ui_find_action(const UIAction* actions, size_t count, const char* name) {
    for (size_t i = 0; i < count; i++)
        if (strcmp(actions[i].name, name) == 0)
            return actions[i].fn;
    return NULL;
}

// This gets a line and converts it into a null terminated string
static void trim_newline(char* s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

// This strips a string surrounded by quotes and removes those
static void strip_quotes(char* s) {
    size_t len = strlen(s);
    // Check if first char and last char is "
    if (len >= 2 && s[0] == '"' && s[len - 1] == '"') {
        // Get all but "
        memmove(s, s + 1, len - 1);
        s[len - 2] = '\0';
    }
}

// Searches for the next token
static char* next_token(char** cursor) {
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

    // Search for quotes
    while (*s) {
        if (*s == '"') {
            inQuotes = !inQuotes;
        }
        // If not in quotes and found delimiter, no more iterating
        else if ((*s == ' ' || *s == '\n' || *s == '\r') && !inQuotes) {
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

// Only checks for the first character
static bool get_bool(char *value) {
    return *value == 't' || *value == 'T';
}

// Get element by its tag, returns NULL if there is no elements with that tag
UIElement *ui_get_element_by_tag(UIScreen *screen, const char *tag) {
    for (int i = 0; i < screen->count; i++) {
        for (int j = 0; j < TAGS_PER_ELEMENT; j++) {
            // Check for element with this tag
            if (strcmp(screen->elements[i].tag[j], tag) == 0) {
                return &screen->elements[i];
            }
        }
    }
    // No element found
    return NULL;
}

// Run a function on each element with an specific tag
void ui_run_func_on_tag(UIScreen *screen, const char *tag, void (*func)(UIElement *e)) {
    for (int i = 0; i < screen->count; i++) {
        for (int j = 0; j < TAGS_PER_ELEMENT; j++) {
            // Check for element with this tag
            if (strcmp(screen->elements[i].tag[j], tag) == 0) {
                func(&screen->elements[i]);
            }
        }
    }
}

// Run a function on each element with an specific tag
void ui_set_pos_on_tag(UIScreen *screen, float x, float y, const char *tag) {
    bool found_parent = false;
    float movement_x = 0;
    float movement_y = 0;
    for (int i = 0; i < screen->count; i++) {
        for (int j = 0; j < TAGS_PER_ELEMENT; j++) {
            UIElement *e = &screen->elements[i];
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

void ui_enable_element(UIElement *e) { 
    e->enabled = true;
};

void ui_disable_element(UIElement *e) { 
    e->enabled = false;
    if (e->type == UI_BUTTON) {
        e->button.hovered = false;
        e->button.hoverScale = 1.f;
        e->button.hoverTimer = 0.f;
    }

    if (e->type == UI_CHECKBOX) {
        e->checkbox.hovered = false;
        e->checkbox.hoverScale = 1.f;
        e->checkbox.hoverTimer = 0.f;
    }
};

void copy_tag_array(UIElement *e, char (*tag)[TAG_LENGTH]) {
    if (tag == NULL) return;

    for (int i = 0; i < TAGS_PER_ELEMENT; i++) {
        strncpy(e->tag[i], tag[i], TAG_LENGTH - 1);
    }
}

static void split_tags(char *input, char tag[][TAG_LENGTH]) {
    int i = 0;
    char *token = strtok(input, ",");

    while (token != NULL && i < TAGS_PER_ELEMENT)
    {
        strncpy(tag[i], token, TAG_LENGTH - 1);
        tag[i][TAG_LENGTH - 1] = '\0';   // ensure null termination
        i++;

        token = strtok(NULL, ",");
    }
}

// Load an screen from its file, needs a pointer to the actions table and the action count
void ui_load_screen(UIScreen* screen,
                    const UIAction* actions,
                    size_t actionCount,
                    const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;

    screen->count = 0;

    char line[512];

    // Iterate through lines (one element per line)
    while (fgets(line, sizeof(line), f)) {

        trim_newline(line);

        // Comment or empty
        if (line[0] == '#' || line[0] == '\0')
            continue;
        
        char* cursor = line;
        char* token = next_token(&cursor);

        // Check for invalid tokens
        if (!token) continue;

        // The element type
        char type[16];
        strncpy(type, token, 15);

        // Default values
        int x = 0, y = 0, id = 0;
        float sx = 1.0f, sy = 1.0f, scale = 1.0f;
        float align = 0.f;
        bool checked = false;
        int style = 0;
        int sheet = 0;
        int w = 0, h = 0;
        int limit = 16;
        float opacity = 1.0f;
        int max_value = 100;

        int index = 1, color_index = 0, gamemode = 0;

        // Some strings
        char actionName[64] = {0};
        char text[256] = {0};
        char tag[TAGS_PER_ELEMENT][TAG_LENGTH] = {0};

        // Parse element parameters
        while ((token = next_token(&cursor)) != NULL) {
            char* equal = strchr(token, '=');
            if (!equal) continue;

            // This replaces the equal sign between key and value with a null character, dividing the string in two
            *equal = '\0';

            char* key = token;
            char* value = equal + 1;

            // Parameters, this might be made better
            if (strcmp(key, "x") == 0)
                x = atoi(value);
            else if (strcmp(key, "y") == 0)
                y = atoi(value);
            else if (strcmp(key, "w") == 0)
                w = atoi(value);
            else if (strcmp(key, "h") == 0)
                h = atoi(value);
            else if (strcmp(key, "id") == 0)
                id = atoi(value);
            else if (strcmp(key, "style") == 0)
                style = atoi(value);
            else if (strcmp(key, "sheet") == 0)
                sheet = atoi(value);
            else if (strcmp(key, "sx") == 0)
                sx = atof(value);
            else if (strcmp(key, "sy") == 0)
                sy = atof(value);
            else if (strcmp(key, "scale") == 0) {
                float s = atof(value);
                scale = s;
                sx = s;
                sy = s;
            } else if (strcmp(key, "action") == 0) {
                strip_quotes(value);
                strncpy(actionName, value, 63);
            } else if (strcmp(key, "text") == 0) {
                strip_quotes(value);
                strncpy(text, value, 255);
            } else if (strcmp(key, "align") == 0) {
                if (strcmp(value, "LEFT") == 0) {
                    align = 0.f;
                } else if (strcmp(value, "CENTER") == 0) {
                    align = 0.5f;
                } else if (strcmp(value, "RIGHT") == 0) {
                    align = 1.0f;
                } else {
                    align = 0.f;
                } 
            } else if (strcmp(key, "tag") == 0) {
                strip_quotes(value);
                split_tags(value, tag);
            } else if (strcmp(key, "checked") == 0) {
                checked = get_bool(value);
            } else if (strcmp(key, "title") == 0) {
                strip_quotes(value);
                strncpy(text, value, 63);
            } else if (strcmp(key, "limit") == 0) {
                limit = atoi(value);
            } else if (strcmp(key, "opacity") == 0) {
                opacity = atof(value);
            } else if (strcmp(key, "index") == 0) {
                index = atoi(value);
            } else if (strcmp(key, "gamemode") == 0) {
                gamemode = atoi(value);
            } else if (strcmp(key, "color_index") == 0) {
                color_index = atoi(value);
            } else if (strcmp(key, "max_value") == 0) {
                max_value = atoi(value);
            }
        }

        // There is a limit of elements on an screen, exit if reached
        if (screen->count >= UI_MAX_ELEMENTS)
            break;

        // Elements
        if (strcmp(type, "button") == 0) {
            screen->elements[screen->count++] =
                ui_create_button(
                    x, y, sx, sy, id, sheet, opacity,
                    ui_find_action(actions, actionCount, actionName),
                    text,
                    tag
                );
        } else if (strcmp(type, "image") == 0) {
            screen->elements[screen->count++] =
                ui_create_image(x, y, id, sheet, sx, sy, tag);
        } else if (strcmp(type, "label") == 0) {
            screen->elements[screen->count++] =
                ui_create_label(x, y, scale, text, align, tag);
        } else if (strcmp(type, "label_aller") == 0) {
            screen->elements[screen->count++] =
                ui_create_label_aller(x, y, scale, text, align, tag);
        } else if (strcmp(type, "checkbox") == 0) {
            screen->elements[screen->count++] =
                ui_create_checkbox(
                    x, y, sx, sy, checked,
                    ui_find_action(actions, actionCount, actionName),
                    tag
                );
        } else if (strcmp(type, "window") == 0) {
            screen->elements[screen->count++] =
                ui_create_window(
                    x, y, w, h, style,
                    tag
                );
        } else if (strcmp(type, "textbox") == 0) {
            screen->elements[screen->count++] =
                ui_create_textbox(
                    x, y, w, limit, text,
                    tag
                );
        } else if (strcmp(type, "list") == 0) {
            screen->elements[screen->count++] =
                ui_create_list(
                    x, y, w, h,
                    tag
                );
        } else if (strcmp(type, "bggradient") == 0) {
            screen->elements[screen->count++] =
                ui_create_bg_gradient(
                    tag
                );
        } else if (strcmp(type, "actionarea") == 0) {
            screen->elements[screen->count++] =
                ui_create_action_area(
                    x, y, w, h,
                    ui_find_action(actions, actionCount, actionName),
                    tag
                );
        } else if (strcmp(type, "darken") == 0) {
            screen->elements[screen->count++] =
                ui_create_darken(
                    opacity, tag
                );
        } else if (strcmp(type, "icon") == 0) {
            screen->elements[screen->count++] =
                ui_create_icon(
                    x, y, scale, index, gamemode,
                    ui_find_action(actions, actionCount, actionName),
                    tag
                );
        } else if (strcmp(type, "colorbutton") == 0) {
            screen->elements[screen->count++] =
                ui_create_color_button(
                    x, y, scale, index, color_index,
                    ui_find_action(actions, actionCount, actionName),
                    tag
                );
        } else if (strcmp(type, "windowbutton") == 0) {
            screen->elements[screen->count++] =
                ui_create_window_button(
                    x, y, w, h, style,
                    ui_find_action(actions, actionCount, actionName),
                    text,
                    tag
                );
        } else if (strcmp(type, "progressbar") == 0) {
            screen->elements[screen->count++] =
                ui_create_progress_bar(
                    x, y, style, scale, max_value,
                    tag
                );
        }
    }

    fclose(f);
}
