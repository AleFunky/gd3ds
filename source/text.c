#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include "math_helpers.h"

#include "text.h"
#include "fonts/bigFont.h"
#include <stdarg.h>

static const u32 white = ABGR8(255, 255, 255, 255);

typedef struct {
    const char *name;
    const u32 color;
} TagColor;

// HOW TO USE TAGS
// like this "<red>red text wohoo</>"
// or this "<#ff0000>also red text wohoo</>"
// you can also do this "<green>i am green <#ff0000>and i am red</>"
// </> ALWAYS resets to white and doesn't care if there is a tag before, its just so it looks like html
// <p> makes a new line

// Even thought the macro is called "ABGR8", the paremeters are still in this order: red, green, blue, alpha
static const TagColor color_table[] = {
    { "red",   ABGR8(255, 0, 0, 255)},
    { "green", ABGR8(0, 255, 0, 255)},
    { "blue",  ABGR8(0, 0, 255, 255)},
};

#define NUM_COLORS sizeof(color_table) / sizeof(color_table[0])

static bool parse_named_color_tag(const char *tag, u32 *out) {
    for (int i = 0; i < NUM_COLORS; i++) {
        if (strcasecmp(tag, color_table[i].name) == 0) {
            *out = color_table[i].color;
            return true;
        }
    }

    return false;
}

static bool parse_hex_color(const char *str, u32 *out) {
    // Please are you a valid color yes or no
    if (strlen(str) != 7 || str[0] != '#') {
        return false;
    }

    unsigned int r, g, b;

    // Parse hex
    if (sscanf(str + 1, "%02x%02x%02x", &r, &g, &b) != 3) {
        return false;
    }

    *out = ABGR8(r, g, b, 255);

    return true;
}

static bool parse_color_tag(const char *tag, u32 *out) {
    // Named colors
    if(parse_named_color_tag(tag, out))
        return true;

    if (strcmp(tag, "/") == 0) {
        *out = white;
        return true;
    }

    // Hex color
    if (tag[0] == '#') {
        return parse_hex_color(tag, out);
    }

    return false;
}

static bool read_tag(const char *text, int *i, char *tag_out, int max) {
    if (text[*i] != '<') {
        return false;
    }

    // Scan after <
    int start = *i + 1;
    int end = start;

    // Scan until >
    while (text[end] && text[end] != '>') {
        end++;
    }

    // If end of text, oops
    if (text[end] != '>') {
        return false;
    }

    int len = end - start;

    if (len >= max) {
        len = max - 1;
    }

    // Get substring
    memcpy(tag_out, &text[start], len);
    tag_out[len] = '\0';

    *i = end;

    return true;
}

// Count da lines
static int count_lines(const char *text) {
    int lines = 1;

    for (int i = 0; text[i]; i++) {
        if (text[i] == '<') {
            char tag[64];
            
            // If found p tag, we found a line separator
            if (read_tag(text, &i, tag, sizeof(tag))) {
                if (strcmp(tag, "p") == 0) {
                    lines++;
                }
            }
        }
    }

    return lines;
}

const Glyph *get_glyph(const Charset *font, char character) {
    // Find matching character
    for (int i = 0; i < font->count; i++) {
        if (character == font->glyphs[i].id) {
            return &font->glyphs[i];
        }
    }

    // If not found and a lowercase letter, convert to uppercase
    if (character >= 'a' && character <= 'z') {
        character -= 32;

        // Search again
        for (int i = 0; i < font->count; i++) {
            if (character == font->glyphs[i].id) {
                return &font->glyphs[i];
            }
        }
    }
    
    return NULL;
}

float get_line_length(const Charset *font, const float zoom_x, const char *text, int start) {
    float text_length = 0;

    for (int i = start; text[i]; i++) {
        const Glyph *character = get_glyph(font, text[i]);

        // Skip tags
        if (text[i] == '<') {
            char tag[64];

            if (read_tag(text, &i, tag, sizeof(tag))) {
                if (strcmp(tag, "p") == 0) {
                    break;
                }

                continue;
            }
        }

        if (character) {
            text_length += character->xAdvance * zoom_x;
        }
    }

    return text_length;
}

float get_text_length(const Charset *font, const float zoom_x, const char *text) {
    float text_length = 0;
    int size = strlen(text);
    for (int i = 0; i < size; i++) {
        const Glyph *character = get_glyph(font, text[i]);
        
        // Skip tags
        if (text[i] == '<') {
            char tag[64];

            if (read_tag(text, &i, tag, sizeof(tag))) {
                continue;
            }
        }

        if (character != NULL) {
            float xadvance = character->xAdvance * zoom_x;

            text_length += xadvance;
        }
    }
    return text_length;
}

#define SPACING 6.f

void draw_text(const Charset *font, C2D_SpriteSheet *sheet, const float x, const float y, const float scale, float alignment, const char *text, ...) {
    if (!text || !sheet) {
        return;
    }

    char tmp[1024];

    va_list argp;
    va_start(argp, text);
    const int size = vsnprintf(tmp, sizeof(tmp), text, argp);
    va_end(argp);

    float height = HEIGHT_OFFSET;

    const Glyph *aCharacter = get_glyph(font, 'A');
    if (aCharacter) {
        height = aCharacter->height * HEIGHT_OFFSET_MULT;
    }

    float line_length = get_line_length(font, fabsf(scale), tmp, 0);

    float offset_x = 0;
    float offset_y = 0;

    // Get total text height
    int line_count = count_lines(tmp) - 1;
    float line_height = (height * scale) + SPACING * scale;
    float total_height = (line_count * line_height) - SPACING * scale;

    C2D_ImageTint tint = { 0 };
    u32 current_color = white;

    for (int i = 0; i < size; i++) {
        // Parse tags
        if (tmp[i] == '<') {
            char tag[64];

            if (read_tag(tmp, &i, tag, sizeof(tag))) {                
                if (strcmp(tag, "p") == 0) {
                    offset_x = 0;
                    
                    offset_y += line_height;

                    // Measure next line
                    line_length = get_line_length(
                        font,
                        fabsf(scale),
                        tmp,
                        i + 1
                    );

                    continue;
                }

                parse_color_tag(tag, &current_color);
                continue;
            }
        }

        C2D_PlainImageTint(&tint, current_color, 1.f);
        
        const Glyph *character = get_glyph(font, tmp[i]);
        
        if (character != NULL) {
            C2D_Sprite sprite = { 0 };

            float xoffset = (character->xOffset) * scale;
            float yoffset = (character->yOffset) * scale;
            float xadvance = character->xAdvance * scale;

            int index = character->spriteIndex;
            
            float base_y = y - total_height / 2.f;
            float final_x = x + offset_x + xoffset - line_length * alignment;
            float final_y = base_y + offset_y + yoffset - (int)(height * scale);

            final_x = (scale == 1.0f) ? roundf(final_x) : final_x;
            final_y = (scale == 1.0f) ? roundf(final_y) : final_y;

            if (index >= 0) { 
                // Draw glyph so its center is at (final_x, final_y)
                C2D_SpriteFromSheet(&sprite, *sheet, index);
                C3D_TexSetFilter(sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
                C2D_SpriteSetPos(&sprite, final_x, final_y);
                C2D_SpriteSetScale(&sprite, scale, scale);
                C2D_DrawSpriteTinted(&sprite, &tint);
            }

            offset_x += xadvance;
        }
    }
}