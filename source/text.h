#pragma once
#include <3ds.h>
#include <citro2d.h>

typedef struct {
    unsigned short id;
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
    short xOffset;
    short yOffset;
    short xAdvance;
    short spriteIndex;
} Glyph;

#define HEIGHT_OFFSET (20.f)
#define HEIGHT_OFFSET_MULT (HEIGHT_OFFSET / 29)

#define TEXT_OBJECT_SCALE (1.0f)

typedef struct {
    const Glyph* glyphs;
    unsigned int count;
} Charset;

typedef struct {
    float x, y;
    short sprite;
} TextGlyphPlacement;

bool parse_hex_color(const char *str, u32 *out);

unsigned char text_object_layout(const Charset *font, const char *text, TextGlyphPlacement *out, unsigned char max_out);

void draw_text(const Charset *font, C2D_SpriteSheet *sheet, const float x, const float y, const float scaleX, const float scaleY, float alignment, bool parse_tags, const char *text, ...);
float get_text_length(const Charset *font, const float zoom_x, bool parse_tags, const char *text);
float get_longest_line_length(const Charset *font, const float zoom_x, const char *text);
char *wrap_text(const Charset *font, float zoom_x, const char *text, float max_width);
int count_lines(const char *text, bool parse_tags);