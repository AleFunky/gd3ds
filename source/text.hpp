#pragma once
#include <3ds.h>
#include <citro2d.h>

namespace gd3ds
{

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

#define TEXT_OBJECT_SCALE (1.0f)

typedef struct {
    const Glyph* glyphs;
    unsigned int count;
    float scale;
} Charset;

typedef struct {
    float x, y;
    short sprite;
} TextGlyphPlacement;

bool parseHexColor(const char *str, u32 *out);

unsigned char textObjectLayout(const Charset *font, const char *text, TextGlyphPlacement *out, unsigned char maxOut);

void drawText(const Charset *font, C2D_SpriteSheet *sheet, const float x, const float y, const float scaleX, const float scaleY, float alignment, bool parseTags, const char *text, ...);
float getTextLength(const Charset *font, const float zoomX, bool parseTags, const char *text);
float getLongestLineLength(const Charset *font, const float zoomX, const char *text);
char *wrapText(const Charset *font, float zoomX, const char *text, float maxWidth);
int countLines(const char *text, bool parseTags);
float getTextHeight(const Charset *font, C2D_SpriteSheet *sheet, const float scaleY, bool parseTags, const char *text);

}
