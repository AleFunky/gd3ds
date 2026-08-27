#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include "math_helpers.h"

#include "text.h"
#include "fonts/bigFont.h"
#include <stdarg.h>

#include "menus/core/ui_screen.h"

static const u32 white = ABGR8(255, 255, 255, 255);
static char wrap_buffer[4096];

typedef struct {
    const char *name;
    const u32 color;
} TagColor;

// HOW TO USE TAGS
// like this "<red>red text wohoo</>"
// or this "<#ff0000>also red text wohoo</>"
// you can also do this "<green>i am green <#ff0000>and i am red</>"
// now you can also put it in decimal like "<255,0,0>red</>" and also include opacity "<255,255,255,127>half</>""
// </> ALWAYS resets to white and doesn't care if there is a tag before, its just so it looks like html
// \n makes a new line
// you can display an image like if it was an emoji via "<i(id)s(s)>", where id is the iamge index and s the spritesheet id, for example, "<i19s1>" display image #19 in sheet #1, which is soggy

// Even thought the macro is called "ABGR8", the paremeters are still in this order: red, green, blue, alpha
static const TagColor color_table[] = {
    { "red",   ABGR8(255, 0, 0, 255)},
    { "green", ABGR8(0, 255, 0, 255)},
    { "blue",  ABGR8(0, 0, 255, 255)},
};

static bool parse_named_color_tag(const char *tag, u32 *out) {
    for (int i = 0; i < ARRAY_LEN(color_table); i++) {
        if (strcasecmp(tag, color_table[i].name) == 0) {
            *out = color_table[i].color;
            return true;
        }
    }

    return false;
}

bool parse_hex_color(const char *str, u32 *out) {
    unsigned int r, g, b, a;

    if (str[0] == '#') {
        // RRGGBBAA
        if (sscanf(str + 1, "%02x%02x%02x%02x", &r, &g, &b, &a) == 4 && strlen(str) == 9) {
            *out = ABGR8(r, g, b, a);
            return true;
        }

        // #RRGGBB
        if (sscanf(str + 1, "%02x%02x%02x", &r, &g, &b) == 3 && strlen(str) == 7) {
            *out = ABGR8(r, g, b, 255);
            return true;
        }

        return false;
    }

    // r,g,b,a
    if (sscanf(str, "%d,%d,%d,%d", &r, &g, &b, &a) == 4) {
        *out = ABGR8(r, g, b, a);
        return true;
    }

    // r,g,b
    if (sscanf(str, "%d,%d,%d", &r, &g, &b) == 3) {
        *out = ABGR8(r, g, b, 255);
        return true;
    }

    return false;
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
    if (parse_hex_color(tag, out)) {
        return true;
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

static bool parse_image_tag(const char *tag, int *image, int *sheet) {
    return sscanf(tag, "i%ds%d", image, sheet) == 2;
}

// Count da lines
static int count_lines(const char *text, bool parse_tags) {
    if (!parse_tags) return 1;
    
    int lines = 1;

    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            lines++;
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

char *wrap_text(const Charset *font, float zoom_x, const char *text, float max_width) {
    wrap_buffer[0] = '\0';

    float line_width = 0.f;
    const char *p = text;
    while (*p) {
        // Copy tags 
        if (*p == '<') {
            const char *start = p;

            while (*p && *p != '>') {
                p++;
            }

            if (*p == '>') {
                p++;
            }

            strncat(wrap_buffer, start, p - start);
            continue;
        }

        // Extract next word
        char word[256];
        int len = 0;

        while (*p && *p != ' ' && *p != '<') {
            if (len < sizeof(word) - 1) {
                word[len++] = *p;
            }
            p++;
        }

        word[len] = '\0';

        float word_width = get_text_length(font, zoom_x, true, word);

        float space_width = get_text_length(font, zoom_x, true, " ");

        if (word_width <= max_width) {
            // Wrap if needed
            if (line_width > 0 && line_width + space_width + word_width > max_width) {
                strcat(wrap_buffer, "\n");
                line_width = 0.0f;
            } else if (line_width > 0) {
                strcat(wrap_buffer, " ");
                line_width += space_width;
            }

            strcat(wrap_buffer, word);
            line_width += word_width;
        } else {
            if (line_width > 0) {
                strcat(wrap_buffer, "\n");
                line_width = 0.f;
            }

            int start = 0;

            while (start < len) {
                int end = start;
                float chunk_width = 0.f;

                // Find the largest chunk that fits
                while (end < len) {
                    char c[2] = { 
                        word[end], '\0' 
                    };

                    float char_width = get_text_length(font, zoom_x, true, c);

                    // If already outside the max width, break
                    if (end > start && chunk_width + char_width > max_width) {
                        break;
                    }

                    chunk_width += char_width;
                    end++;
                }

                // Append this chunk
                strncat(wrap_buffer, word + start, end - start);

                start = end;

                if (start < len) {
                    strcat(wrap_buffer, "\n");
                }

                line_width = chunk_width;
            }
        }

        // Skip original spaces
        while (*p == ' ') {
            p++;
        }
    }

    return wrap_buffer;
}

static float get_image_tag_length(const Charset *font, const float zoom_x, char *tag) {
    int image_index = -1;
    int image_sheet = 0;
    if (parse_image_tag(tag, &image_index, &image_sheet)) {
        float height = HEIGHT_OFFSET;

        const Glyph *aCharacter = get_glyph(font, 'A');
        if (aCharacter) {
            height = aCharacter->height * HEIGHT_OFFSET_MULT;
        }

        C2D_Image image = C2D_SpriteSheetGetImage(*get_sheet(image_sheet), image_index);
        if (!image.subtex) return 0;

        float tex_w = image.subtex->width;
        float tex_h = image.subtex->height;

        float image_scale = height / tex_h;
        
        float image_scale_x = zoom_x * image_scale;

        float image_width = tex_w * image_scale_x;
        return image_width;
    }

    return 0;
}

static void get_line_metrics(const Charset *font, float zoom_x, bool parse_tags, const char *text, int start, float *out_length, float *out_first_char, float *out_last_char) {
    float length = 0.f;
    float first_char = 0.f, last_char = 0.f;
    bool first = true;

    for (int i = start; text[i]; i++) {
        // Skip tags
        if (text[i] == '<' && parse_tags) {
            char tag[64];
            if (read_tag(text, &i, tag, sizeof(tag))) {
                length += get_image_tag_length(font, zoom_x, tag);
                
                continue;
            }
        } else if (text[i] == '\n') {
            break;
        }

        const Glyph *character = get_glyph(font, text[i]);
        if (character) {
            // First character
            if (first) { 
                first_char = character->xOffset * zoom_x; 
                first = false; 
            }

            // Calculate gap between visual end and xadvance end
            last_char = (character->xAdvance - character->xOffset - character->width) * zoom_x;
            length += character->xAdvance * zoom_x;
        }
    }

    *out_length = length;
    *out_first_char = first_char;
    *out_last_char = last_char;
}

float get_line_length(const Charset *font, const float zoom_x, bool parse_tags, const char *text, int start) {
    float text_length = 0;

    for (int i = start; text[i]; i++) {
        const Glyph *character = get_glyph(font, text[i]);

        // Skip tags
        if (text[i] == '<' && parse_tags) {
            char tag[64];

            if (read_tag(text, &i, tag, sizeof(tag))) {
                text_length += get_image_tag_length(font, zoom_x, tag);

                continue;
            }
        } else if (text[i] == '\n') {
            break;
        }

        if (character) {
            text_length += character->xAdvance * zoom_x;
        }
    }

    return text_length;
}

float get_longest_line_length(const Charset *font, const float zoom_x, const char *text) {
    float longest = 0.0f;

    int start = 0;
    int i = 0;
    while (true) {
        if (text[i] == '<' || text[i] == '\0') {
            bool newline = false;
            
            if (text[i] == '\n') {
                newline = true;
            }

            // Measure line
            if (newline || text[i] == '\0') {
                float length = get_line_length(font, zoom_x, true, text, start);

                // Set if longer than last saved line
                if (length > longest) {
                    longest = length;
                }

                start = i + 1;
            }
        }

        // Break if end of text
        if (text[i] == '\0') {
            break;
        }

        i++;
    }

    return longest;
}

float get_text_length(const Charset *font, const float zoom_x, bool parse_tags, const char *text) {
    float text_length = 0;
    int size = strlen(text);
    for (int i = 0; i < size; i++) {
        const Glyph *character = get_glyph(font, text[i]);
        
        // Skip tags
        if (text[i] == '<' && parse_tags) {
            char tag[64];

            if (read_tag(text, &i, tag, sizeof(tag))) {
                text_length += get_image_tag_length(font, zoom_x, tag);
                continue;
            }
        } else if (text[i] == '\n') {
            continue;
        }

        if (character != NULL) {
            float xadvance = character->xAdvance * zoom_x;

            text_length += xadvance;
        }
    }
    return text_length;
}

#define SPACING 6.f

void draw_text(const Charset *font, C2D_SpriteSheet *sheet, const float x, const float y, const float scaleX, const float scaleY, float alignment, bool parse_tags, const char *text, ...) {
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

    float line_length;
    float first_char;
    float last_char;

    get_line_metrics(font, fabsf(scaleX), parse_tags, tmp, 0, &line_length, &first_char, &last_char);

    float offset_x = 0;
    float offset_y = 0;

    // Get total text height
    int line_count = count_lines(tmp, parse_tags) - 1;
    float line_height = (height * fabsf(scaleY)) + SPACING * fabsf(scaleY);
    float total_height = (line_count * line_height);

    C2D_ImageTint tint = { 0 };
    u32 current_color = white;

    for (int i = 0; i < size; i++) {
        bool is_image = false;
        int image_index = -1;
        int image_sheet = 0;
        
        // Parse tags
        if (tmp[i] == '<' && parse_tags) {
            char tag[64];

            if (read_tag(tmp, &i, tag, sizeof(tag))) {   
                if (parse_image_tag(tag, &image_index, &image_sheet)) {
                    is_image = true;
                } else {
                    parse_color_tag(tag, &current_color);
                    continue;
                }
            }
        } else if (tmp[i] == '\n') {
            offset_x = 0;
            
            offset_y += line_height;

            get_line_metrics(font, fabsf(scaleX), true, tmp, i + 1, &line_length, &first_char, &last_char);

            continue;
        }

        C2D_PlainImageTint(&tint, current_color, 1.f);
        
        float width_offset = alignment * (line_length - first_char - last_char) + first_char;

        if (!is_image) {
            const Glyph *character = get_glyph(font, tmp[i]);
            
            if (character != NULL) {
                C2D_Sprite sprite = { 0 };

                float xoffset = (character->xOffset) * scaleX;
                float yoffset = (character->yOffset) * scaleY;
                float xadvance = character->xAdvance * scaleX;

                int index = character->spriteIndex;
                
                float base_y = y - total_height / 2.f;

                float final_x = x + offset_x + xoffset - width_offset;
                float final_y = base_y + offset_y + yoffset - height * scaleY;

                final_x += (character->width  * scaleX) * 0.5f;
                final_y += (character->height * scaleY) * 0.5f;

                if (index >= 0) { 
                    // Draw glyph so its center is at (final_x, final_y)
                    C2D_SpriteFromSheet(&sprite, *sheet, index);
                    C3D_TexSetFilter(sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
                    C2D_SpriteSetCenter(&sprite, 0.5f, 0.5f);
                    C2D_SpriteSetPos(&sprite, final_x, final_y);
                    C2D_SpriteSetScale(&sprite, scaleX, scaleY);
                    C2D_DrawSpriteTinted(&sprite, &tint);
                }

                offset_x += xadvance;
            }
        } else {
            // Math slop
            C2D_Sprite sprite = { 0 };

            if (image_index >= 0) { 
                C2D_Image image = C2D_SpriteSheetGetImage(*get_sheet(image_sheet), image_index);
                if (image.subtex) {
                    C2D_SpriteFromSheet(&sprite, *get_sheet(image_sheet), image_index);
                    C3D_TexSetFilter(sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
                
                    float tex_w = sprite.image.subtex->width;
                    float tex_h = sprite.image.subtex->height;

                    float image_scale = height / tex_h;
                    
                    float image_scale_x = scaleX * image_scale;
                    float image_scale_y = scaleY * image_scale;

                    float image_width = tex_w * image_scale_x;
                    float image_height = tex_h * image_scale_y;

                    float xadvance = image_width;
                    
                    float base_y = y - total_height / 2.f;
                    float final_x = x + offset_x - width_offset;
                    float final_y = base_y + offset_y - image_height * 0.5f;

                    final_x += image_width * 0.5f;
                    final_y += image_height * 0.5f;

                    // Draw image so its center is at (final_x, final_y)
                    C2D_SpriteSetCenter(&sprite, 0.5f, 0.5f);
                    C2D_SpriteSetPos(&sprite, final_x, final_y);
                    C2D_SpriteSetScale(&sprite, image_scale_x, image_scale_y);
                    C2D_DrawSpriteTinted(&sprite, &tint);
                    
                    offset_x += xadvance;
                }
            }
        }
    }
}