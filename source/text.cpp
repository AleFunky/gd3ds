#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <3ds/util/utf.h>

#include "math_helpers.hpp"

#include "text.hpp"
#include "fonts/bigFont.hpp"
#include <stdarg.h>
#include <string.h>

#include "menus/core/ui_screen.hpp"

static const u32 white_c = ABGR8(255, 255, 255, 255);
static char wrapBuffer[4096];

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
static const TagColor colorTable[] = {
    { "red",   ABGR8(255, 0, 0, 255)},
    { "green", ABGR8(0, 255, 0, 255)},
    { "blue",  ABGR8(0, 0, 255, 255)},
};

static bool parseNamedColorTag(const char *tag, u32 *out) {
    for (size_t i = 0; i < ARRAY_LEN(colorTable); i++) {
        if (strcasecmp(tag, colorTable[i].name) == 0) {
            *out = colorTable[i].color;
            return true;
        }
    }

    return false;
}

bool parseHexColor(const char *str, u32 *out) {
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

static bool parseColorTag(const char *tag, u32 *out) {
    // Named colors
    if(parseNamedColorTag(tag, out))
        return true;

    if (strcmp(tag, "/") == 0) {
        *out = white_c;
        return true;
    }

    // Hex color
    if (parseHexColor(tag, out)) {
        return true;
    }

    return false;
}

static bool readTag(const char *text, int *i, char *tagOut, int max) {
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
    memcpy(tagOut, &text[start], len);
    tagOut[len] = '\0';

    *i = end;

    return true;
}

static bool parseImageTag(const char *tag, int *image, int *sheet) {
    return sscanf(tag, "i%ds%d", image, sheet) == 2;
}

// Count da lines
int countLines(const char *text, bool parseTags) {
    if (!parseTags) return 1;
    
    int lines = 1;

    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            lines++;
        }
    }

    return lines;
}

static int decodeUtf8Codepoint(const char *text, size_t remaining, uint32_t *codepoint) {
    uint8_t sequence[4] = { 0 };
    size_t available = remaining < sizeof(sequence) ? remaining : sizeof(sequence);
    memcpy(sequence, text, available);

    ssize_t bytes = decode_utf8(codepoint, sequence);
    if (bytes <= 0 || (size_t)bytes > remaining) {
        *codepoint = '?'; // robert when binaryVersion is not 49
        return 1;
    }

    return bytes;
}

const Glyph *getGlyph(const Charset *font, uint32_t character) {
    // Find matching character
    for (size_t i = 0; i < font->count; i++) {
        if (character == font->glyphs[i].id) {
            return &font->glyphs[i];
        }
    }

    // If not found and a lowercase letter, convert to uppercase
    if (character >= 'a' && character <= 'z') {
        character -= 32;

        // Search again
        for (size_t i = 0; i < font->count; i++) {
            if (character == font->glyphs[i].id) {
                return &font->glyphs[i];
            }
        }
    }
    
    return NULL;
}

char *wrapText(const Charset *font, float zoomX, const char *text, float maxWidth) {
    wrapBuffer[0] = '\0';

    zoomX *= font->scale;

    float lineWidth = 0.f;
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

            strncat(wrapBuffer, start, p - start);
            continue;
        }

        // Extract next word
        char word[256];
        size_t len = 0;
        size_t remaining = strlen(p);

        while (remaining && *p != ' ' && *p != '<') {
            uint32_t codepoint;
            int bytes = decodeUtf8Codepoint(p, remaining, &codepoint);
            if (len + bytes < sizeof(word)) {
                memcpy(word + len, p, bytes);
                len += bytes;
            }
            p += bytes;
            remaining -= bytes;
        }

        word[len] = '\0';

        float wordWidth = getTextLength(font, zoomX, true, word);

        float spaceWidth = getTextLength(font, zoomX, true, " ");

        if (wordWidth <= maxWidth) {
            // Wrap if needed
            if (lineWidth > 0 && lineWidth + spaceWidth + wordWidth > maxWidth) {
                strcat(wrapBuffer, "\n");
                lineWidth = 0.0f;
            } else if (lineWidth > 0) {
                strcat(wrapBuffer, " ");
                lineWidth += spaceWidth;
            }

            strcat(wrapBuffer, word);
            lineWidth += wordWidth;
        } else {
            if (lineWidth > 0) {
                strcat(wrapBuffer, "\n");
                lineWidth = 0.f;
            }

            size_t start = 0;

            while (start < len) {
                size_t end = start;
                float chunkWidth = 0.f;

                // Find the largest chunk that fits
                while (end < len) {
                    uint32_t codepoint;
                    int bytes = decodeUtf8Codepoint(word + end, len - end, &codepoint);
                    const Glyph *glyph = getGlyph(font, codepoint);
                    float charWidth = glyph ? glyph->xAdvance * zoomX : 0.f;

                    // If already outside the max width, break
                    if (end > start && chunkWidth + charWidth > maxWidth) {
                        break;
                    }

                    chunkWidth += charWidth;
                    end += bytes;
                }

                // Append this chunk
                strncat(wrapBuffer, word + start, end - start);

                start = end;

                if (start < len) {
                    strcat(wrapBuffer, "\n");
                }

                lineWidth = chunkWidth;
            }
        }

        // Skip original spaces
        while (*p == ' ') {
            p++;
        }
    }

    return wrapBuffer;
}

static float getFontLineHeight(const Charset *font) {
    const Glyph *reference = getGlyph(font, 'A');
    return reference ? reference->height : 29.f;
}

static float getFontImageCenterY(const Charset *font) {
    const Glyph *reference = getGlyph(font, 'A');
    return reference ? reference->yOffset + reference->height * 0.5f : getFontLineHeight(font) * 0.5f;
}

static float getImageTagLength(const Charset *font, const float zoomX, char *tag) {
    int imageIndex = -1;
    int imageSheet = 0;
    if (parseImageTag(tag, &imageIndex, &imageSheet)) {
        float height = getFontLineHeight(font);
        
        C2D_SpriteSheet sheet = getSheet(imageSheet);
        if (!sheet) return 0;

        C2D_Image image = C2D_SpriteSheetGetImage(sheet, imageIndex);
        if (!image.subtex) return 0;

        float texW = image.subtex->width;
        float texH = image.subtex->height;

        float imageScale = height / texH;
        
        float imageScaleX = zoomX * imageScale;

        float imageWidth = texW * imageScaleX;
        return imageWidth;
    }

    return 0;
}

static void getLineMetrics(const Charset *font, float zoomX, bool parseTags, const char *text, int start, float *outLength, float *outFirstChar, float *outLastChar) {
    float length = 0.f;
    float firstChar = 0.f, lastChar = 0.f;
    bool first = true;

    int textSize = strlen(text);
    for (int i = start; i < textSize;) {
        if (text[i] == '<' && parseTags) {
            char tag[64];
            int tagEnd = i;
            if (readTag(text, &tagEnd, tag, sizeof(tag))) {
                length += getImageTagLength(font, zoomX, tag);
                i = tagEnd + 1;
                continue;
            }
        }

        if (text[i] == '\n') {
            break;
        }

        uint32_t codepoint;
        size_t bytes = decodeUtf8Codepoint(text + i, textSize - i, &codepoint);
        const Glyph *character = getGlyph(font, codepoint);
        if (character) {
            // First character
            if (first) { 
                firstChar = character->xOffset * zoomX; 
                first = false; 
            }

            // Calculate gap between visual end and xadvance end
            lastChar = (character->xAdvance - character->xOffset - character->width) * zoomX;
            length += character->xAdvance * zoomX;
        }
        i += bytes;
    }

    *outLength = length;
    *outFirstChar = firstChar;
    *outLastChar = lastChar;
}

float getLineLength(const Charset *font, const float zoomX, bool parseTags, const char *text, size_t start) {
    float textLength = 0;
    int textSize = strlen(text);

    for (int i = start; i < textSize;) {
        // Skip tags
        if (text[i] == '<' && parseTags) {
            char tag[64];
            int tagEnd = i;
            if (readTag(text, &tagEnd, tag, sizeof(tag))) {
                textLength += getImageTagLength(font, zoomX, tag);
                i = tagEnd + 1;
                continue;
            }
        }

        if (text[i] == '\n') {
            break;
        }

        uint32_t codepoint;
        size_t bytes = decodeUtf8Codepoint(text + i, textSize - i, &codepoint);
        const Glyph *character = getGlyph(font, codepoint);
        if (character) {
            textLength += character->xAdvance * zoomX;
        }
        i += bytes;
    }

    return textLength;
}

float getLongestLineLength(const Charset *font, const float zoomX, const char *text) {
    float longest = 0.0f;

    size_t start = 0;
    size_t textSize = strlen(text);
    for (size_t i = 0; i <= textSize; i++) {
        if (i < textSize && text[i] == '<') {
            char tag[64];
            int tagEnd = i;
            if (readTag(text, &tagEnd, tag, sizeof(tag))) {
                i = tagEnd;
                continue;
            }
        }

        if (i == textSize || text[i] == '\n') {
            float length = getLineLength(font, zoomX, true, text, start);
            if (length > longest) {
                longest = length;
            }
            start = i + 1;
        }
    }

    return longest;
}

float getTextLength(const Charset *font, const float zoomX, bool parseTags, const char *text) {
    float textLength = 0;
    size_t size = strlen(text);
    for (size_t i = 0; i < size;) {
        // Skip tags
        if (text[i] == '<' && parseTags) {
            char tag[64];
            int tagEnd = i;
            if (readTag(text, &tagEnd, tag, sizeof(tag))) {
                textLength += getImageTagLength(font, zoomX, tag);
                i = tagEnd + 1;
                continue;
            }
        }

        if (text[i] == '\n') {
            i++;
            continue;
        }

        uint32_t codepoint;
        int bytes = decodeUtf8Codepoint(text + i, size - i, &codepoint);
        const Glyph *character = getGlyph(font, codepoint);
        if (character != NULL) {
            float xadvance = character->xAdvance * zoomX;
            textLength += xadvance;
        }
        i += bytes;
    }
    return textLength;
}

#define SPACING 6.f

static void includeVerticalBounds(float top, float bottom, bool *hasBounds, float *minY, float *maxY) {
    if (!(*hasBounds)) {
        *minY = top;
        *maxY = bottom;
        *hasBounds = true;
        return;
    }

    if (top < *minY) *minY = top;
    if (bottom > *maxY) *maxY = bottom;
}

static void getTextVerticalBounds(const Charset *font, const char *text, size_t size, bool parseTags, float scaleY, float lineHeight, float *minY, float *maxY) {
    float offsetY = 0.f;
    bool hasBounds = false;

    for (size_t i = 0; i < size;) {
        uint32_t codepoint;
        int bytes = decodeUtf8Codepoint(text + i, size - i, &codepoint);

        if (codepoint == '\n') {
            i += bytes;
            offsetY += lineHeight;
            continue;
        }

        bool isImage = false;
        int imageIndex = -1;
        int imageSheet = 0;

        if (codepoint == '<' && parseTags) {
            char tag[64];
            int tagEnd = i;
            if (readTag(text, &tagEnd, tag, sizeof(tag))) {
                i = tagEnd + 1;
                if (parseImageTag(tag, &imageIndex, &imageSheet)) {
                    isImage = true;
                } else {
                    continue;
                }
            } else {
                i += bytes;
            }
        } else {
            i += bytes;
        }

        if (isImage) {
            C2D_SpriteSheet imageSheetPtr = getSheet(imageSheet);
            if (imageIndex >= 0 && imageSheetPtr) {
                C2D_Image image = C2D_SpriteSheetGetImage(imageSheetPtr, imageIndex);
                if (image.subtex) {
                    float imageHeight = getFontLineHeight(font) * fabsf(scaleY);
                    float imageCenter = offsetY + getFontImageCenterY(font) * scaleY;
                    includeVerticalBounds(imageCenter - imageHeight * 0.5f, imageCenter + imageHeight * 0.5f, &hasBounds, minY, maxY);
                }
            }
            continue;
        }

        const Glyph *character = getGlyph(font, codepoint);
        if (character) {
            float glyphCenter = offsetY + (character->yOffset + character->height * 0.5f) * scaleY;
            float glyphHalfHeight = character->height * fabsf(scaleY) * 0.5f;
            
            includeVerticalBounds(glyphCenter - glyphHalfHeight, glyphCenter + glyphHalfHeight, &hasBounds, minY, maxY);
        }
    }

    if (!hasBounds) {
        *minY = 0.f;
        *maxY = 0.f;
    }
}

float getTextHeight(const Charset *font, C2D_SpriteSheet *sheet, const float scaleY, bool parseTags, const char *text) {
    int size = strlen(text);
    float modScaleY = scaleY * font->scale;

    float height = getFontLineHeight(font);
    float lineHeight = height * 0.9f * fabsf(scaleY);

    float minY;
    float maxY;
    getTextVerticalBounds(font, text, size, parseTags, modScaleY, lineHeight, &minY, &maxY);
    return maxY - minY;
}

void drawText(const Charset *font, C2D_SpriteSheet *sheet, const float x, const float y, const float scaleX, const float scaleY, float alignment, bool parseTags, const char *text, ...) {
    if (!text || !sheet) {
        return;
    }

    char tmp[1024];

    va_list argp;
    va_start(argp, text);
    const size_t size = (size_t)vsnprintf(tmp, sizeof(tmp), text, argp);
    va_end(argp);

    float lineLength;
    float firstChar;
    float lastChar;

    float modScaleX = scaleX * font->scale;
    float modScaleY = scaleY * font->scale;

    getLineMetrics(font, fabsf(modScaleX), parseTags, tmp, 0, &lineLength, &firstChar, &lastChar);

    float offsetX = 0;
    float offsetY = 0;

    float height = getFontLineHeight(font);
    float lineHeight = height * 0.9f * fabsf(scaleY);

    float minY;
    float maxY;
    getTextVerticalBounds(font, tmp, size, parseTags, modScaleY, lineHeight, &minY, &maxY);
    float baseY = y - (minY + maxY) * 0.5f;

    C2D_ImageTint tint = { 0 };
    u32 currentColor = white_c;

    for (size_t i = 0; i < size;) {
        uint32_t codepoint;
        int bytes = decodeUtf8Codepoint(tmp + i, size - i, &codepoint);
        bool isImage = false;
        int imageIndex = -1;
        int imageSheet = 0;
        
        // Parse tags
        if (codepoint == '<' && parseTags) {
            char tag[64];
            int tagEnd = i;
            if (readTag(tmp, &tagEnd, tag, sizeof(tag))) {
                i = tagEnd + 1;
                if (parseImageTag(tag, &imageIndex, &imageSheet)) {
                    isImage = true;
                } else {
                    parseColorTag(tag, &currentColor);
                    continue;
                }
            } else {
                i += bytes;
            }
        } else if (codepoint == '\n') {
            i += bytes;
            offsetX = 0;
            
            offsetY += lineHeight;

            getLineMetrics(font, fabsf(modScaleX), parseTags, tmp, i, &lineLength, &firstChar, &lastChar);

            continue;
        } else {
            i += bytes;
        }

        const Glyph *character = getGlyph(font, codepoint);
        C2D_PlainImageTint(&tint, currentColor, 1.f);
        
        float widthOffset = alignment * (lineLength - firstChar - lastChar) + firstChar;

        if (!isImage) {
            if (character != NULL) {
                C2D_Sprite sprite = { 0 };

                float xoffset = (character->xOffset) * modScaleX;
                float yoffset = (character->yOffset) * modScaleY;
                float xadvance = character->xAdvance * modScaleX;

                int index = character->spriteIndex;
                
                float finalX = x + offsetX + xoffset - widthOffset;
                float finalY = baseY + offsetY + yoffset;

                finalX += (character->width  * modScaleX) * 0.5f;
                finalY += (character->height * modScaleY) * 0.5f;

                if (index >= 0) { 
                    // Draw glyph so its center is at (final_x, final_y)
                    C2D_SpriteFromSheet(&sprite, *sheet, index);
                    C3D_TexSetFilter(sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
                    C2D_SpriteSetCenter(&sprite, 0.5f, 0.5f);
                    C2D_SpriteSetPos(&sprite, finalX, finalY);
                    C2D_SpriteSetScale(&sprite, modScaleX, modScaleY);
                    C2D_DrawSpriteTinted(&sprite, &tint);
                }

                offsetX += xadvance;
            }
        } else {
            // Math slop
            C2D_Sprite sprite = { 0 };

            if (imageIndex >= 0) { 
                C2D_SpriteSheet sheet = getSheet(imageSheet);
                if (!sheet) continue;

                C2D_Image image = C2D_SpriteSheetGetImage(sheet, imageIndex);
                if (image.subtex) {
                    C2D_SpriteFromSheet(&sprite, sheet, imageIndex);
                    C3D_TexSetFilter(sprite.image.tex, GPU_LINEAR, GPU_LINEAR);
                
                    float texW = sprite.image.subtex->width;
                    float texH = sprite.image.subtex->height;

                    float imageScale = height / texH;
                    
                    float imageScaleX = modScaleX * imageScale;
                    float imageScaleY = modScaleY * imageScale;

                    float imageWidth = texW * imageScaleX;

                    float xadvance = imageWidth;
                    
                    float finalX = x + offsetX - widthOffset;
                    float finalY = baseY + offsetY + getFontImageCenterY(font) * modScaleY;

                    finalX += imageWidth * 0.5f;

                    // Draw image so its center is at (final_x, final_y)
                    C2D_SpriteSetCenter(&sprite, 0.5f, 0.5f);
                    C2D_SpriteSetPos(&sprite, finalX, finalY);
                    C2D_SpriteSetScale(&sprite, imageScaleX, imageScaleY);
                    C2D_DrawSpriteTinted(&sprite, &tint);
                    
                    offsetX += xadvance;
                }
            }
        }
    }
}

unsigned char textObjectLayout(const Charset *font, const char *text, TextGlyphPlacement *out, unsigned char maxOut) {
    if (!font || !text || !out || maxOut == 0) return 0;

    float width = 0.f;
    for (int i = 0; text[i]; i++) {
        const Glyph *g = getGlyph(font, text[i]);
        if (g) width += g->xAdvance;
    }
    width *= TEXT_OBJECT_SCALE;

    const Glyph *reference = getGlyph(font, 'A');
    float lineHeight = reference ? (float)reference->height : 29.f;

    float cursor = -width * 0.5f;
    unsigned char count = 0;

    for (int i = 0; text[i]; i++) {
        const Glyph *g = getGlyph(font, text[i]);
        if (!g) continue;

        if (g->spriteIndex >= 0 && count < maxOut) {
            out[count].x = cursor + (g->xOffset + g->width * 0.5f) * TEXT_OBJECT_SCALE;
            out[count].y = (g->yOffset + g->height * 0.5f) * TEXT_OBJECT_SCALE - lineHeight * TEXT_OBJECT_SCALE * 0.5f;
            out[count].sprite = g->spriteIndex;
            count++;
        }

        cursor += g->xAdvance * TEXT_OBJECT_SCALE;
    }

    return count;
}