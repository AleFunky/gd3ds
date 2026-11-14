#include "graphics.hpp"
#include "level_loading.hpp"
#include "objects.hpp"
#include "animations.hpp"
#include "main.hpp"
#include "math_helpers.hpp"
#include "player/robot_anim_data.hpp"
#include "player/spider_anim_data.hpp"
#include "triggers.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mp3_player.hpp"
#include "icons.hpp"
#include "icon_cache.hpp"
#include "menus/icon_kit.hpp"
#include "menus/palette_kit.hpp"

#include "player/player.hpp"
#include "profiling.hpp"
#include "state.hpp"
#include "player/collision.hpp"

#include "utils/gfx.hpp"

#include "particles/object_particles.hpp"
#include "particles/circles.hpp"
#include "particles/coin_effect.hpp"
#include "particles/collectable_effect.hpp"

#include "particles/gravity_sweep.hpp"

#include "menus/settings_hub/settings.hpp"

#include "menus/core/ui_screen.hpp"

#include "particles/lightning.hpp"

#include "fonts/level_fonts.hpp"
#include "particles/rays.hpp"
#include "practice.hpp"

namespace gd3ds
{

extern const Color white = { 255, 255, 255 };

#define COLLECTABLE_FLOAT_AMP 1.5f
#define COLLECTABLE_FLOAT_PERIOD 0.8f
#define GAMEPLAY_SCREEN_WIDTH_AREA SCREEN_WIDTH_AREA_GAMEPLAY(settingsState.swapGameplayScreens && !inLevelComplete)
#define COLLECTABLE_FLOAT_BASE 0.0f

static inline float collectableFloatOffset(void) {
    return COLLECTABLE_FLOAT_BASE + COLLECTABLE_FLOAT_AMP * lutSin(6.2831853f * (frameTimer / COLLECTABLE_FLOAT_PERIOD));
}

static const HSV lighterHSV = {
    .h = 0.0f,
    .s = 0.65f,
    .v = 1.30f,
    .sChecked = false,
    .vChecked = false
};

static bool colorEqual(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

static Color applyHSVToColor(Color src, int gameObject, bool isMain, int colChannel) {
    bool *valid;
    Color *cachedSrc, *cachedColor;
    HSV *hsv;

    if (isMain) {
        hsv = &objects.mainColHSV[gameObject];
        valid = &objects.cachedMainHSVValid[gameObject];
        cachedSrc = &objects.cachedMainHSVSrcColor[gameObject];
        cachedColor = &objects.cachedMainHSVColor[gameObject];
    } else {
        hsv = &objects.detailColHSV[gameObject];
        valid = &objects.cachedDetailHSVValid[gameObject];
        cachedSrc = &objects.cachedDetailHSVSrcColor[gameObject];
        cachedColor = &objects.cachedDetailHSVColor[gameObject];
    }

    if (*valid && colorEqual(*cachedSrc, src)) {
        return *cachedColor;
    }

    Color out = hsvCombine(src, *hsv);
    *cachedSrc = src;
    *cachedColor = out;
    *valid = true;

    return out;
}

static size_t spriteCount = 0;

static bool blendingState = false;

C2D_SpriteSheet spriteSheet;
C2D_SpriteSheet spriteSheet2;
C2D_SpriteSheet spriteSheet3;
C2D_SpriteSheet spriteSheet4;
C2D_SpriteSheet animatedSheet;
C2D_SpriteSheet playerEffectsSheet;
C2D_SpriteSheet glowSheet;
C2D_SpriteSheet bgSheet;
int loadedBgSheet = 0;
C2D_SpriteSheet groundSheet;
C2D_SpriteSheet trailSheet;
C2D_SpriteSheet particleSheet;

static SortItem bufA[MAX_SPRITES];
static SortItem bufB[MAX_SPRITES];

static SpriteObject viewableObjects[MAX_SPRITES];
static SpriteObject *viewableObjectsPtr[MAX_SPRITES];
static int currentObjects[MAX_SPRITES];
static int currentObjectCount;

bool p1Trail = false;
float p1TrailTimer = 0;
int currentFadingEffect = static_cast<int>(FadingEffects::FadeSimple);
int currentPulserodBallImage = 0;

SpriteTemplate spriteTemplates[GAME_OBJECT_COUNT]; // global cache

float touchEffectDragTimer = 0.f;

static C2D_SpriteSheet getSpriteSheet(int index, int *relIndex) {
    // Check if index belongs to spritesheet 1 (most objects)
    if (index < SPRITESHEET2_START) {
        *relIndex = index;
        return spriteSheet;
    }

    if (index < SPRITESHEET3_START) {
        // Return spritesheet 2 (portals)
        *relIndex = index - SPRITESHEET2_START;
        return spriteSheet2;
    }

    if (index < SPRITESHEET4_START) {
        // Return spritesheet 3 (2.0 objects)
        *relIndex = index - SPRITESHEET3_START;
        return spriteSheet3;
    }

    if (index < ANIMATEDSHEET_START) {
        // Return spritesheet 4 (2.1 objects)
        *relIndex = index - SPRITESHEET4_START;
        return spriteSheet4;
    }

    // Return spritesheet 5 (animated objects)
    *relIndex = index - ANIMATEDSHEET_START;
    return animatedSheet;
}

C2D_SpriteSheet getSpriteSheetEx(int index, int *relIndex) {
    return getSpriteSheet(index, relIndex);
}

const SlotFrames* findSlotFrames(const GameObject* obj, int slot) {
    for (size_t i = 0; i < obj->slotCount; i++) {
        if (obj->slotFrames[i].slot == slot) return &obj->slotFrames[i];
    }
    return NULL;
}

size_t getChildGroup(const GameObject* obj, size_t childIndex) {
    for (size_t g = 0; g < obj->groupCount; g++) {
        size_t end = obj->groups[g].start + obj->groups[g].count;
        if (childIndex >= obj->groups[g].start && childIndex < end)
            return g;
    }
    return -1;
}

const Animation* getAnimationForObject(int id) {
    switch (id) {
        case 918: return &animations[static_cast<int>(AnimationID::AnimGjbeast01Bite)];
        case 919: return &animations[static_cast<int>(AnimationID::AnimBlacksludgeLoop)];
        case 1327: return &animations[static_cast<int>(AnimationID::AnimGjbeast02Idle01)];
        case 1328: return &animations[static_cast<int>(AnimationID::AnimGjbeast03Idle01)];
        default:  return NULL;
    }
}

Color getColorAbgr8(u32 color) {
    Color col;
    col.r = R_ABGR8(color);
    col.g = G_ABGR8(color);
    col.b = B_ABGR8(color);

    return col;
}

void updatePlayerColors() {
    Color p1 = getColorAbgr8(colors[selectedP1]);
    Color p2 = getColorAbgr8(colors[selectedP2]);
    Color glow = getColorAbgr8(colors[selectedGlow]);

    setPlayerColors(p1, p2, glow);
}

Color getWhiteIfBlack(Color color) {
    if ((color.r | color.g | color.b) == 0) return white;
    
    return color;
}

// Gets p1 color accounting for p1 and p2 being black
Color getP2IfBlack(Color color) {
    // Check if p1 is black
    if ((color.r | color.g | color.b) == 0) {
        // If p1 is also black, return white
        if ((p2Color.r | p2Color.g | p2Color.b) == 0) {
            return white;
        }
        // Else just return p2
        return p2Color;
    }
    
    return color;
}

// Gets p2 color accounting for p1 and p2 being black
Color getP1IfBlack(Color color) {
    // Check if p2 is black
    if ((color.r | color.g | color.b) == 0) {
        // If p1 is also black, return white
        if ((p1Color.r | p1Color.g | p1Color.b) == 0) {
            return white;
        }
        // Else return p1
        return p1Color;
    }
    
    return color;
}

void setPlayerColors(Color p1, Color p2, Color glow) {
    p1Color = p1;
    p2Color = p2;
    glowColor = glow;
    
    // Update particles
    Color p1NotWhite = getWhiteIfBlack(p1Color);

    touchDragParticles.cfg.startColorRed = p1NotWhite.r / 255.f;
    touchDragParticles.cfg.startColorGreen = p1NotWhite.g / 255.f;
    touchDragParticles.cfg.startColorBlue = p1NotWhite.b / 255.f;

    touchExplosionParticles.cfg.startColorRed = p1NotWhite.r / 255.f;
    touchExplosionParticles.cfg.startColorGreen = p1NotWhite.g / 255.f;
    touchExplosionParticles.cfg.startColorBlue = p1NotWhite.b / 255.f;

    glitterParticlesBottom.cfg.startColorRed = p1NotWhite.r / 255.f;
    glitterParticlesBottom.cfg.startColorGreen = p1NotWhite.g / 255.f;
    glitterParticlesBottom.cfg.startColorBlue = p1NotWhite.b / 255.f;
    glitterParticlesBottom.cfg.finishColorRed = p1NotWhite.r / 255.f;
    glitterParticlesBottom.cfg.finishColorGreen = p1NotWhite.g / 255.f;
    glitterParticlesBottom.cfg.finishColorBlue = p1NotWhite.b / 255.f;
}

// This might make sprite making faster, im not sure thought
void cacheAllSprites() {
    for (int id = 0; id < GAME_OBJECT_COUNT; id++) {
        const GameObject* obj = (const GameObject*)&gameObjects[id];

        // Get glow frame
        if (obj->glowFrame >= 0) {
            C2D_SpriteFromSheet(&spriteTemplates[id].glowTemplate, glowSheet, obj->glowFrame);
            C3D_TexSetFilter(spriteTemplates[id].glowTemplate.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&spriteTemplates[id].glowTemplate, 0.5f, 0.5f);
        }

        // Skip if object has no texture
        if (obj->texture < 0 && obj->childCount == 0) continue;

        // parent template (if object has a parent textur
        if (obj->texture >= 0) {
            int tex;
            C2D_SpriteSheet sheet = getSpriteSheet(obj->texture, &tex);

            C2D_SpriteFromSheet(&spriteTemplates[id].parentTemplate, sheet, tex);
            C3D_TexSetFilter(spriteTemplates[id].parentTemplate.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&spriteTemplates[id].parentTemplate, 0.5f, 0.5f);
        }

        // Children
        spriteTemplates[id].childCount = obj->childCount;
        if (obj->childCount > 0) {
            spriteTemplates[id].childTemplates = (C2D_Sprite *)malloc(sizeof(C2D_Sprite) * obj->childCount);
            for (size_t i = 0; i < obj->childCount; i++) {
                const ChildSprite* c = &obj->children[i];
                if (c->texture < 0) continue;

                int cTex;
                C2D_SpriteSheet cSheet = getSpriteSheet(c->texture, &cTex);

                C2D_SpriteFromSheet(&spriteTemplates[id].childTemplates[i], cSheet, cTex);
                C3D_TexSetFilter(spriteTemplates[id].childTemplates[i].image.tex, GPU_LINEAR, GPU_LINEAR);
                C2D_SpriteSetCenter(&spriteTemplates[id].childTemplates[i], 0.5f, 0.5f);
            }
        } else {
            spriteTemplates[id].childTemplates = NULL;
        }
    }
}

void freeCachedSprites() {
    for (int i = 0; i < GAME_OBJECT_COUNT; i++) {
        if (spriteTemplates[i].childTemplates)
            free(spriteTemplates[i].childTemplates);
    }
}

float mirrorAngle(float angle, bool hflip, bool vflip) {
    if (hflip && vflip) {
        angle += 180.0f;
    } else if (hflip) {
        angle = 180.0f - angle;
    } else if (vflip) {
        angle = -angle;
    }

    return normalizeAngle(angle);
}

// Returns true if the object is a invisible object
bool objectFades(int obj) {
    switch (objects.id[obj]) {
        case 144:
        case 145:
        case 146:
        case 147:
        case 204:
        case 205:
        case 206:
        case 459:
        case 673:
        case 674:
        case 740:
        case 741:
        case 742:
            return true;
    }
    return false;
}

int getGlowChannel(int obj);

inline int getColorChannel(int colType, int obj, const GameObject *gameObj) {
    int objID = objects.id[obj];
    int colChannel = gameObj->baseColor;
    if (colType == static_cast<int>(ColorType::ColorTypeGlow)) {
        colChannel = getGlowChannel(obj);
        if (colChannel != static_cast<int>(ColorChannelIDs::ChannelObjBlending)) return colChannel;
    } 
    
    // Check for the presence of 1.9 color channel
    if (objects.v1p9ColChannel[obj]) {
        // If pulserods, use base instead of detail
        if (objID >= 15 && objID <= 17) {
            if (colType != static_cast<int>(ColorType::ColorTypeDetail)) colChannel = objects.v1p9ColChannel[obj];
        } else {
            if (colType == static_cast<int>(ColorType::ColorTypeDetail)) colChannel = objects.v1p9ColChannel[obj];
        }
    } else {
        // 2.0 color channels
        if (objects.colChannel[obj]) {
            if (colType != static_cast<int>(ColorType::ColorTypeDetail)) {
                colChannel = objects.colChannel[obj];
            } else if (!objHasMain(gameObj)) {
                colChannel = objects.colChannel[obj];
            }
        }

        if (objects.detailColChannel[obj]) {
            if (colType == static_cast<int>(ColorType::ColorTypeDetail)) {
                if (objHasMain(gameObj)) {
                    colChannel = objects.detailColChannel[obj];
                }
            }
        }

        
        // Pulserods can have obj channel but be p1
        if (objID >= 15 && objID <= 17) {
            if (colType == static_cast<int>(ColorType::ColorTypeBase) && colChannel == static_cast<int>(ColorChannelIDs::ChannelObj)) {
                colChannel = static_cast<int>(ColorChannelIDs::ChannelP1);
            }
        }
    }

    return colChannel;
}

// Baby's first reverse engineered function

float getFadingObjFade(int obj, float rightEdge, float *glowOut) {
    if (!state.dead) {
        float camera_width = GAMEPLAY_SCREEN_WIDTH_AREA;
        float leftFadeBound = (camera_width / 2) - 75.f;
        float leftFadeWidth = leftFadeBound - 30;
        float rightFadeBound = leftFadeBound + 110;
        float rightFadeWidth = camera_width - (leftFadeBound + 190);

        // Offset fade checks slightly so invisible blocks
        // begin fading before reaching the actual boundary
        float objX = objects.x[obj];
        float marginX = objX;
        if (objX <= state.cameraXMiddle) {
            marginX += 0;//object->m_fadeMargin;
        } else {
            marginX -= 0;//object->m_fadeMargin;
        }
        objX = marginX;

        float halfCameraWidth = (camera_width / 2);
        float camX = state.cameraX;
        
        // Additional screen-edge fade so objects near the
        // far edges of the screen become less visible
        float edgeFactor;
        float distanceFromCenter;
        if (objX <= halfCameraWidth + camX) {
            // Left fade
            edgeFactor = 0.014285714f;
            distanceFromCenter = ((halfCameraWidth + camX) - objX);
        } else {
            // Right fade
            edgeFactor = 0.02f;
            distanceFromCenter = (objX - camX) - halfCameraWidth;
        }
        
        // Convert edge distance into a normalized visibility factor
        float visibilityScale = (halfCameraWidth - distanceFromCenter) * edgeFactor;
        float edgeVisibilityFactor = CLAMP(visibilityScale, 0.0f, 1.0f);

        // Compute fade distance from the invisible region bounds
        float distanceFromFade;
        float fadeWidth;

        if (marginX <= camX + rightFadeBound) {
            // Left fade
            distanceFromFade = (camX + leftFadeBound) - marginX;
            fadeWidth = leftFadeWidth;
        } else {
            // Right fade
            distanceFromFade = (marginX - camX) - rightFadeBound;
            fadeWidth = rightFadeWidth;
        }

        // Set a minimum of 1
        if (fadeWidth <= 1.0f) {
            fadeWidth = 1.0f;
        }
        
        // Minimum opacity is 5%
        float fadeAlpha = CLAMP(distanceFromFade / fadeWidth, 0.0f, 1.0f);
        int objectOpacity = (fadeAlpha * 0.95f + 0.05f) * 255;
        
        int edgeVisibility = edgeVisibilityFactor * 255;
        if (objectOpacity >= edgeVisibility) {
            objectOpacity = edgeVisibility;
        }

        int glowOpacity = (fadeAlpha * 0.85f + 0.15f) * 255;
        if (glowOpacity >= edgeVisibility) {
            glowOpacity = edgeVisibility;
        }

       *glowOut = glowOpacity / 255.f;
        return objectOpacity / 255.f;
    }

    return 1.f;
}

// Get the glow color channel
int getGlowChannel(int obj) {
    if (objectFades(obj)) {
        return static_cast<int>(ColorChannelIDs::ChannelInvisibleGlow);
    }

    int id = objects.id[obj];
    switch (id) {
        case 143:
        case 177:
        case 178:
        case 179:
        case 183:
        case 184:
        case 185:
        case 186:
        case 187:
        case 188:
        case 918:
        case 1327:
        case 1328:
            return static_cast<int>(ColorChannelIDs::ChannelLbgNolerp);
        case 144:
        case 145:
        case 146:
        case 147:
        case 204:
        case 205:
        case 206:
        case 459:
        case 673:
        case 674:
        case 740:
        case 741:
        case 742:
            return static_cast<int>(ColorChannelIDs::ChannelLbg);
        case YELLOW_PAD:
        case YELLOW_ORB:
            return static_cast<int>(ColorChannelIDs::ChannelYellowGlowInternal);
        case BLUE_PAD:
        case BLUE_ORB:
            return static_cast<int>(ColorChannelIDs::ChannelBlueGlow);
        case PINK_PAD:
        case PINK_ORB:
            return static_cast<int>(ColorChannelIDs::ChannelPinkGlow);
        case 200:
        case 201:
        case 202:
        case 203:
            return static_cast<int>(ColorChannelIDs::ChannelWhiteGlow);
        case 397:
        case 398:
        case 399:
        case 675:
        case 676:
        case 677:
            return static_cast<int>(ColorChannelIDs::ChannelLbg);

    }
    return static_cast<int>(ColorChannelIDs::ChannelObjBlending);
}

int getCoinTexture(int tex, int ticks) {
    return tex + ((levelFrame / ticks) & 0b11);;
}

// Some objects have a randomized texture at level load, get those
int getObjRandomLayer(int obj, int id) {
    int tex = gameObjects[id].texture;
    switch (id) {
        case 9: {
            int offset = objects.random[obj] & 0b11;
            if (offset == 3) offset = 0;
            
            if (offset > 0) offset += 3;

            return tex + offset;
        }
        case 135: {
            return tex + (objects.random[obj] & 0b11);
        }
        
        case SECRET_COIN:
            return getCoinTexture(tex + (state.customLevel ? (isCoinCollected(obj) ? 8 : 4) : (isCoinCollected(obj) ? 12 : 0)), 26);
    }
    return -1;
}

// Deco saws rotate slower than normal saws. If not a saw, rotation speed is just 0
float getRotationSpeed(int obj) {
    switch (objects.id[obj]) {
        case 88: 
        case 89:
        case 98:
        case 183:
        case 184:
        case 185:
        case 186:
        case 187:
        case 188:
        case 397:
        case 398:
        case 399:
        case 675:
        case 676:
        case 677:
        case 678:
        case 679:
        case 680:
        case 740:
        case 741:
        case 742:
        case 1705:
        case 1706:
        case 1707:
        case 1708:
        case 1709:
        case 1710:
        case 1734:
        case 1735:
        case 1736:
        case 1752:
            return 360.f;
        
        case 85:
        case 86:
        case 87:
        case 97:
        case 137:
        case 138:
        case 139:
        case 154:
        case 155:
        case 156:
        case 180:
        case 181:
        case 182:
        case 222:
        case 223:
        case 224:
        case 375:
        case 376:
        case 377:
        case 378:
        case 394:
        case 395:
        case 396:
        case 997:
        case 998:
        case 999:
        case 1000:
        case 1055:
        case 1056:
        case 1057:
        case GREEN_ORB:
        case BLACK_ORB:
        case 1521:
        case 1525:
        case 1582:
            return 180.f;
        case 1522:
        case 1526:
            return 150.f;
        case 1523:
        case 1527:
            return 120.f;
        case 1524:
        case 1528:
            return 80.f;
        case 1019:
            return 180.f + mapRange(objects.random[obj] & 0xff, 0, 255, -10, 10);
        case 1020:
            return 100.f + mapRange(objects.random[obj] & 0xff, 0, 255, -10, 10);
        case 1021:
            return 80.f + mapRange(objects.random[obj] & 0xff, 0, 255, -10, 10);
        case 1058:
        case 1059:
        case 1060:
        case 1061:
        case 1831:
        case 1832:
        case 1833:
        case 1834:
            return 300.f;
        case 1619:
        case 1620:
            return 720;
    }
    return 0.f;
}

// Map amplitude pulsing to ranges
float getObjectPulse(float amplitude, int id, int layer) {
     // No pulse if one of those is 0
    amplitude *= musicVolume > 0 && globalVolume > 0;
    amplitude = MAX(0.1f, amplitude); // Cap at 0.1
    switch (id) {
        case YELLOW_ORB:
        case BLUE_ORB:
        case PINK_ORB:
        case GREEN_ORB:
        case BLACK_ORB:
        case RED_ORB:
        case DASH_ORB:
        case GRAVITY_DASH_ORB:
        case TOGGLE_ORB:
            return mapRange(amplitude, 0.f, 1.f, 0.3f, 1.2f);
        case 15:
        case 16:
        case 17:
            if (layer == 2) {    
                return amplitude;
            }
            return 1.0f;
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 60:
        case 148:
        case 149:
        case 405:
            return amplitude;
        case 132:
        case 133:
        case 136:
        case 150:
        case 236:
        case 460:
        case 494:
        case 495:
        case 496:
        case 497:
            return mapRange(amplitude, 0.f, 1.f, 0.6f, 1.2f);
    }
    return 1.0f;
}

int getColorType(const GameObject *gameObj, int obj, int colType) {
    // Check for the presence of 1.9 color channel
    if (objects.v1p9ColChannel[obj]) {
        colType = static_cast<int>(ColorType::ColorTypeBase);
    } else {
        if (colType == static_cast<int>(ColorType::ColorTypeDetail)) {
            if (!objHasMain(gameObj)) {
                colType = gameObj->swapBaseDetail ? static_cast<int>(ColorType::ColorTypeBase) : static_cast<int>(ColorType::ColorTypeOnlyDetail);
            }
        }
    }
    return colType;
}

void spawnObjectAt(
    int objGame,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flipX,
    unsigned char flipY,
    float scale
) {
    const GameObject* obj = (const GameObject*)&gameObjects[id];

    float rad = C3D_AngleFromDegrees(adjustAngle(deg, 0, state.mirrorMult < 0));
    float cosR = lutCos(rad);
    float sinR = lutSin(rad);

    int flipXMult = (flipX ? -1 : 1);
    int flipYMult = (flipY ? -1 : 1);

    float m00 = cosR;
    float m01 = sinR;
    float m10 = sinR;
    float m11 = -cosR;

    float objScaleX = objects.scaleX[objGame];
    float objScaleY = objects.scaleY[objGame];

    float sx = scale * flipXMult * objScaleX;
    float sy = scale * flipYMult * objScaleY;

    // get anim for this object
    const AnimFrame* anim_keyframe = NULL;
    if (obj->animationType == static_cast<int>(AnimationType::AnimationMovement) && obj->groupCount > 0) {
        const Animation* anim = getAnimationForObject(id);
        if (anim && anim->frameCount > 0) {
            float time = frameTimer * anim->fps;
            anim_keyframe = &anim->frames[(int)time % anim->frameCount];
        }
    }

    if (spriteCount >= MAX_SPRITES - 1) return;

    if (id == COUNTER_LABEL || id == TEXT_OBJECT) {
        const TextGlyphPlacement *glyphs = NULL;
        int glyphCount = 0;

        if (id == COUNTER_LABEL) {
            CounterLabel *label = getCounterLabel(objGame);
            if (!label) return;
            int value = getItemCount(label->itemID);
            if (label->text[0] == 0 || value != label->lastValue) {
                snprintf(label->text, sizeof(label->text), "%d", value);
                label->lastValue = value;
                label->layoutDone = 0;
            }
            if (!label->layoutDone) {
                label->glyphCount = textObjectLayout(levelFont, label->text, label->glyphs, MAX_COUNTER_TEXT);
                label->layoutDone = 1;
            }
            glyphs = label->glyphs;
            glyphCount = label->glyphCount;
        } else {
            TextObject *textObj = getTextObject(objGame);
            if (textObj->len > 0) {
                if (!textObj->layoutDone) {
                    textObj->glyphCount = textObjectLayout(levelFont, textObj->text, textObj->glyphs, MAX_TEXT_LEN);
                    textObj->layoutDone = 1;
                }
                glyphs = textObj->glyphs;
                glyphCount = textObj->glyphCount;
            }
        }

        for (int i = 0; i < glyphCount; i++) {
            if (spriteCount >= MAX_SPRITES - 1) break;

            const TextGlyphPlacement *place = &glyphs[i];
            SpriteObject *vo = &viewableObjects[spriteCount];

            vo->hidden = false;

            float localX = place->x * flipXMult;
            float localY = place->y * flipYMult;

            float rotX = localX * cosR - localY * sinR;
            float rotY = localX * sinR + localY * cosR;

            C2D_SpriteFromSheet(&vo->spr, levelFontSheet ? levelFontSheet : bigFontSheet, place->sprite);
            C3D_TexSetFilter(vo->spr.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);
            C2D_SpriteSetPos(&vo->spr, x + rotX * scale * objScaleX, y + rotY * scale * objScaleY);
            C2D_SpriteSetScale(&vo->spr, TEXT_OBJECT_SCALE * sx, TEXT_OBJECT_SCALE * sy);
            C2D_SpriteSetRotation(&vo->spr, rad);

            vo->obj = objGame;
            vo->layer = 0;
            vo->colType = getColorType(obj, objGame, obj->colorType);
            vo->opacity = obj->opacity;
            vo->colChannel = getColorChannel(obj->colorType, objGame, obj);
            calcQuadParams(vo);
            viewableObjectsPtr[spriteCount] = vo;

            spriteCount++;
        }
        return;
    }

    // Spawn parent, skip if no texture
    if (obj->texture >= 0) {
        SpriteObject *vo = &viewableObjects[spriteCount];

        vo->hidden = false;

        float localX = obj->x * flipXMult;
        float localY = obj->y * flipYMult;

        float rotX = localX * m00 + localY * m01;
        float rotY = localX * m10 + localY * m11;

        float pX = x + rotX * scale * objScaleX;
        float pY = y + rotY * scale * objScaleY;
        
        if (obj->animationType == static_cast<int>(AnimationType::AnimationFrameSwap) && obj->frameCount > 0) {
            const SlotFrames* slotFrames = findSlotFrames(obj, 0);
            if (slotFrames) {
                float time = frameTimer * slotFrames->fps;
                int index = (int)time % slotFrames->count;
                const SwapFrame* swapFrame = &obj->swapFrames[slotFrames->start + index];

                int relIndex;
                C2D_SpriteSheet sheet = getSpriteSheet(swapFrame->texture, &relIndex);
                C2D_SpriteFromSheet(&vo->spr, sheet, relIndex);
                C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);
                
                sx *= (swapFrame->flipX ? -1 : 1);
                sy *= (swapFrame->flipY ? -1 : 1);
            } else {
                vo->spr = spriteTemplates[id].parentTemplate;
            }
        } else {
            int randomLayer = getObjRandomLayer(objGame, id);
            if (randomLayer < 0) {
                vo->spr = spriteTemplates[id].parentTemplate;
            } else {
                int relIndex;
                C2D_SpriteSheet sheet = getSpriteSheet(randomLayer, &relIndex);
                C2D_Sprite rnd = { 0 };
                vo->spr = rnd;
                C2D_SpriteFromSheet(&vo->spr, sheet, relIndex);
                C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);
            }
        }

        float pulseScale = getObjectPulse(amplitude, id, 0);

        C2D_SpriteSetPos(&vo->spr, pX, pY);
        C2D_SpriteSetScale(&vo->spr, sx * pulseScale, sy * pulseScale);
        C2D_SpriteSetRotation(&vo->spr, rad);

        vo->obj = objGame;
        vo->layer = 0;
        vo->colType = getColorType(obj, objGame, obj->colorType);
        vo->opacity = obj->opacity;
        vo->colChannel = getColorChannel(obj->colorType, objGame, obj);
        calcQuadParams(vo);
        viewableObjectsPtr[spriteCount] = vo;

        spriteCount++;
    }

    // Skip if no glow frame
    if (settingsState.glowEnabled && obj->glowFrame >= 0) {
        if (spriteCount >= MAX_SPRITES - 1) return;

        SpriteObject *vo = &viewableObjects[spriteCount];

        vo->hidden = false;

        vo->spr = spriteTemplates[id].glowTemplate;

        float pulseScale = getObjectPulse(amplitude, id, 1);

        C2D_SpriteSetPos(&vo->spr, x, y);
        C2D_SpriteSetScale(&vo->spr, sx * pulseScale, sy * pulseScale);
        C2D_SpriteSetRotation(&vo->spr, rad);

        vo->obj = objGame;
        vo->layer = 1;
        vo->colType = static_cast<int>(ColorType::ColorTypeGlow);
        vo->opacity = obj->opacity;
        vo->blending = true;
        vo->colChannel = getColorChannel(static_cast<int>(ColorType::ColorTypeGlow), objGame, obj);
        calcQuadParams(vo);
        viewableObjectsPtr[spriteCount] = vo;
        spriteCount++;
    }

    // Spawn children
    for (size_t i = 0; i < obj->childCount; i++) {
        const ChildSprite* c = &obj->children[i];
        
        if (spriteCount >= MAX_SPRITES - 1) return;
        
        // Skip if no texture
        if (c->texture >= 0) {    
            SpriteObject *vo = &viewableObjects[spriteCount];

            vo->hidden = false;

            float cLocalX = c->x * flipXMult;
            if ((id == BLUE_TP_PORTAL || id == ORANGE_TP_PORTAL) && i == 0)
                cLocalX += 2.0f * flipXMult;
            float cLocalY = c->y * flipYMult;

            float cRotX = cLocalX * m00 + cLocalY * m01;
            float cRotY = cLocalX * m10 + cLocalY * m11;

            float cX = x + cRotX * scale * objScaleX;
            float cY = y + cRotY * scale * objScaleY;

            int cFlipXMult = (c->flipX ? -1 : 1);
            int cFlipYMult = (c->flipY ? -1 : 1);

            float cRot = C3D_AngleFromDegrees(c->rot) + rad;
            float cSx = c->scaleX * sx;
            float cSy = c->scaleY * sy;

            // handle movement anims
            if (anim_keyframe) {
                size_t group = getChildGroup(obj, i);
                if (group >= 0) {
                    const AnimSprite* anim_sprite = NULL;
                    for (size_t k = 0; k < anim_keyframe->spriteCount; k++) {
                        if (anim_keyframe->sprites[k].childSlot == (size_t)group) {
                            anim_sprite = &anim_keyframe->sprites[k];
                            break;
                        }
                    }

                    if (anim_sprite) {
                        float aLocalX = anim_sprite->x * flipXMult;
                        float aLocalY = anim_sprite->y * flipYMult;

                        float aRotX = aLocalX * m00 + aLocalY * m01;
                        float aRotY = aLocalX * m10 + aLocalY * m11;

                        cX = x + aRotX * scale * objScaleX;
                        cY = y + aRotY * scale * objScaleY;

                        cRot = C3D_AngleFromDegrees(anim_sprite->rot) * (flipXMult * flipYMult) + rad;

                        cSx *= anim_sprite->scaleX;
                        cSy *= anim_sprite->scaleY;

                        cFlipXMult ^= anim_sprite->flipX;
                        cFlipYMult ^= anim_sprite->flipY;
                    }
                }
            }
            // handle frame swap anims
            if (obj->animationType == static_cast<int>(AnimationType::AnimationFrameSwap) && obj->frameCount > 0) {
                const SlotFrames* slotFrames = findSlotFrames(obj, i + 1);
                if (slotFrames) {
                    float time = frameTimer * slotFrames->fps;
                    int index = (int)time % slotFrames->count;
                    const SwapFrame* swapFrame = &obj->swapFrames[slotFrames->start + index];

                    int relIndex;
                    C2D_SpriteSheet sheet = getSpriteSheet(swapFrame->texture, &relIndex);
                    C2D_SpriteFromSheet(&vo->spr, sheet, relIndex);
                    C2D_SpriteSetCenter(&vo->spr, 0.5f, 0.5f);

                    cSx *= (swapFrame->flipX ? -1 : 1);
                    cSy *= (swapFrame->flipY ? -1 : 1);
                } else {
                    if (!spriteTemplates[id].childTemplates) continue;
                    vo->spr = spriteTemplates[id].childTemplates[i];
                }
            } else {
                if (!spriteTemplates[id].childTemplates) continue;
                vo->spr = spriteTemplates[id].childTemplates[i];
            }

            float pulseScale = getObjectPulse(amplitude, id, i + 2);

            C2D_SpriteSetPos(&vo->spr, cX, cY);
            if (id < 15 || id > 17) {
                C2D_SpriteSetScale(&vo->spr, cSx * cFlipXMult * pulseScale,
                                          cSy * cFlipYMult * pulseScale);
                C2D_SpriteSetRotation(&vo->spr, cRot);
            } else {
                C2D_SpriteSetScale(&vo->spr, fabsf(cSx * cFlipXMult * pulseScale),
                                          fabsf(cSy * cFlipYMult * pulseScale));
            }

            vo->obj = objGame;
            vo->layer = i + 2;
            vo->colType = getColorType(obj, objGame, c->colorType);
            vo->opacity = c->opacity;
            vo->colChannel = getColorChannel(c->colorType, objGame, obj);
            calcQuadParams(vo);
            viewableObjectsPtr[spriteCount] = vo;
            spriteCount++;
        }
    }
}

static inline uint32_t makeSortKey(SpriteObject *s)
{
    const int obj = s->obj;

    // Player sprite is -1 so handle it there
    if (obj == -1) {
        return ((4 + 8) << 18) | (2 << 16) | (255 << 8) | 128;
    }

    const int id = objects.id[obj];
    const GameObject *gameObj = (const GameObject*)&gameObjects[id];

    int zlayer = objects.zlayer[obj];
    int zorder = objects.zorder[obj];

    bool blending;
    // Blending makes zlayer one 
    if (objHasMain(gameObj) && objHasDetail(gameObj)) {
        bool blendingMain = (objects.colChannel[obj] > 0 && (channels[getColChannelIndex(objects.colChannel[obj])].blending ^ ((zlayer & 1) == 0)));
        bool blendingDetail = (objects.detailColChannel[obj] > 0 && (channels[getColChannelIndex(objects.detailColChannel[obj])].blending ^ ((zlayer & 1) == 0)));
        blending = blendingMain && blendingDetail;
    } else {
        int colChannel = s->colChannel;
        blending = colChannel > 0 && (channels[getColChannelIndex(colChannel)].blending ^ ((zlayer & 1) == 0));
    }

    // If layer is a glow layer or it has blending, decrement it
    if (s->layer == 1 || blending) {
        zlayer--;
    }

    int childZ = 0;
    int tex = gameObj->texture;

    // If layer is a glow layer, it does something for sure
    if (s->layer > 1) {
        const ChildSprite *child = &gameObj->children[s->layer - 2];
        childZ = child->z - 1;
        tex = child->texture;
        zlayer += child->zLayerOffset;
    }
    
    // Glow layers always use spritesheet 2 (only for sorting purposes)
    int sheet;
    if (s->layer == 1) {
        sheet = 0;
    } else {
        sheet = tex < SPRITESHEET2_START || tex >= SPRITESHEET3_START ? 1 : 2;
        // Some animated object are in sheet 3
        switch (id) {
            case 918:
            case 1327:
            case 1328:
            case 920:
            case 921:
            case 923:
            case 924:
                sheet = 3;
                break;
        }
    }

    // Move the pulserod ball
    if (id >= 15 && id <= 17 && s->layer == 2) {
        zlayer += 2;
    } 

    s->zlayer = zlayer;

    // Pack all variables into a nice 32 bit variable
    uint32_t zl = (uint32_t)(zlayer + 8);     // fits in 6 bits
    uint32_t zs = (uint32_t)(3 - sheet);      // fits in 2 bit
    uint32_t zo = (uint32_t)(zorder + 128);   // fits in 8 bits
    uint32_t cz = (uint32_t)(childZ + 128);  // fits in 8 bits

    return (zl << 18) | (zs << 16) | (zo << 8) | cz;
}

void sortViewableObjects(SpriteObject **objects, int count) {
    if (count <= 1) return;

    for (int i = 0; i < count; i++) {
        bufA[i].obj = objects[i];
        bufA[i].key = makeSortKey(objects[i]);
    }

    SortItem *src = bufA;
    SortItem *dst = bufB;

    for (int pass = 0; pass < 3; pass++) {
        uint16_t buckets[256] = {0}; // Crum buckets, speak to da weeb, began duh uh oh oh, oh, oh oh oh oh wiguwiguwi
        int shift = pass * 8;

        for (int i = 0; i < count; i++) {
            buckets[(src[i].key >> shift) & 0xFF]++;
        }

        uint16_t sum = 0;
        for (int i = 0; i < 256; i++) {
            uint16_t t = buckets[i];
            buckets[i] = sum;
            sum += t;
        }

        for (int i = 0; i < count; i++) {
            uint8_t b = (src[i].key >> shift) & 0xFF;
            dst[buckets[b]++] = src[i];
        }

        SortItem *tmp = src;
        src = dst;
        dst = tmp;
    }

    for (int i = 0; i < count; i++) {
        objects[i] = src[i].obj;
    }
}

int getObjectLayers(int id) {
    int count = 0;
    if (id < 0 || id >= GAME_OBJECT_COUNT) return 0;

    const GameObject *obj = (const GameObject*)&gameObjects[id];
    if (obj->texture >= 0) count++;
    
    for (size_t c = 0; c < obj->childCount; c++) {
        if (obj->children[c].texture >= 0) count++;
    }
    return count;
}

float objEdgeFade(float x, int rightEdge) {
    if (x < 0 || x > rightEdge)
        return 0;
    else if (x < FADE_WIDTH)
        return 255.0f * (x / FADE_WIDTH);
    else if (x > rightEdge - FADE_WIDTH)
        return 255.0f * ((rightEdge - x) / FADE_WIDTH);
    else
        return 255;
}

float getXyFadeOffset(float x, int rightEdge) {
    float fade = objEdgeFade(x, rightEdge);
    return (255 - fade) / 2;
}

float getInScaleFade(float x, int rightEdge) {
    float fade = objEdgeFade(x, rightEdge);
    return (fade / 255.f);
}

float getOutScaleFade(float x, int rightEdge) {
    float fade = 255 - objEdgeFade(x, rightEdge);
    return 1 + ((fade / 255.f) / 2);
}

// Some objects dont change opacity on fade transitions
int getObjOpacity(int obj, float x) {
    if ((objects.flags[obj] & static_cast<int>(ObjectFlags::FlagDontFade)) && objects.transitionApplied[obj] == static_cast<int>(FadingEffects::FadeSimple)) return 255;

    float opacity = objEdgeFade(x, GAMEPLAY_SCREEN_WIDTH_AREA);
    bool blending;

    switch (objects.id[obj]) {
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 309:
        case 311:
        case 687:
        case 688:
            if (objects.transitionApplied[obj] == static_cast<int>(FadingEffects::FadeSimple)) opacity = 255;
            break;
            
        case 211:
            blending = channels[getColChannelIndex(objects.colChannel[obj])].blending;
            if (!blending && objects.transitionApplied[obj] == static_cast<int>(FadingEffects::FadeSimple)) opacity = 255;
            break;
        case 207:
        case 208:
        case 209:
        case 210:
        case 212:
        case 213:
        case 693:
        case 694:
        case 331:
        case 333:
            blending = channels[getColChannelIndex(objects.detailColChannel[obj])].blending;
            if (!blending && objects.transitionApplied[obj] == static_cast<int>(FadingEffects::FadeSimple)) opacity = 255;
            break;
    }

    return opacity;
}

// Handle complex fading transitions
void handleSpecialFading(int obj, float calcX, float calcY) {
    switch (currentFadingEffect) {
        case static_cast<int>(FadingEffects::FadeInwards):
            if (calcY > (SCREEN_HEIGHT / SCALE / 2)) {
                objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeUp);
            } else {
                objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeDown);
            }
            break;
        case static_cast<int>(FadingEffects::FadeOutwards):
            if (calcY > (SCREEN_HEIGHT / SCALE / 2)) {
                objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeDown);
            } else {
                objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeUp);
            }
            break;
        case static_cast<int>(FadingEffects::FadeCircleLeft):
            if (calcX > (GAMEPLAY_SCREEN_WIDTH_AREA / 2)) {
                if (calcY > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeUpStationary);
                } else {
                    objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeDownStationary);
                }
            } else {
                if (calcY > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeUpSlowLeft);
                } else {
                    objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeDownSlowLeft);
                }
            }
            break;
        case static_cast<int>(FadingEffects::FadeCircleRight):
            if (calcX > (GAMEPLAY_SCREEN_WIDTH_AREA / 2)) {
                if (calcY > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeUpSlowRight);
                } else {
                    objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeDownSlowRight);
                }
            } else {
                if (calcY > (SCREEN_HEIGHT / SCALE / 2)) {
                    objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeUpStationary);
                } else {
                    objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeDownStationary);
                }
            }
            break;
        default:
            objects.transitionApplied[obj] = currentFadingEffect;  
    }   
}

void getFadeVars(int obj, float x, float *fadeX, float *fadeY, float *fadeScale) {
    if (objects.flags[obj] & static_cast<int>(ObjectFlags::FlagDontEnter)) return;

    switch (objects.transitionApplied[obj]) {
        case static_cast<int>(FadingEffects::FadeSimple):
            break;
        case static_cast<int>(FadingEffects::FadeUp):
            *fadeY = getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            break;
        case static_cast<int>(FadingEffects::FadeDown):
            *fadeY = -getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            break;
        case static_cast<int>(FadingEffects::FadeRight):
            *fadeX = getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            break;
        case static_cast<int>(FadingEffects::FadeLeft):
            *fadeX = -getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            break;
        case static_cast<int>(FadingEffects::FadeScaleIn):
            *fadeScale = getInScaleFade(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            break;
        case static_cast<int>(FadingEffects::FadeScaleOut):
            *fadeScale = getOutScaleFade(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            break;
        case static_cast<int>(FadingEffects::FadeUpSlowLeft):
            *fadeX = -getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            *fadeY = getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA) / 3;
            break;
        case static_cast<int>(FadingEffects::FadeUpSlowRight):
            *fadeX = getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            *fadeY = getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA) / 3;
            break;
        case static_cast<int>(FadingEffects::FadeUpStationary):
            *fadeY = getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA) / 3;
            break;
        case static_cast<int>(FadingEffects::FadeDownSlowLeft):
            *fadeX = -getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            *fadeY = -getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA) / 3;
            break;
        case static_cast<int>(FadingEffects::FadeDownSlowRight):
            *fadeX = getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA);
            *fadeY = -getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA) / 3;
            break;
        case static_cast<int>(FadingEffects::FadeDownStationary):
            *fadeY = -getXyFadeOffset(x, GAMEPLAY_SCREEN_WIDTH_AREA) / 3;
            break;
    }
}

float getSpecialFadingVars(int obj, float fadeVal) {
    if (objects.flags[obj] & static_cast<int>(ObjectFlags::FlagDontEnter)) return 0;

    if (objects.transitionApplied[obj] == static_cast<int>(FadingEffects::FadeDownStationary) || objects.transitionApplied[obj] == static_cast<int>(FadingEffects::FadeUpStationary)) {
        if (fadeVal < 255) {
            float calcX = objects.x[obj] - state.cameraX;
            if (calcX > (GAMEPLAY_SCREEN_WIDTH_AREA) / 2) {
                return (GAMEPLAY_SCREEN_WIDTH_AREA - FADE_WIDTH) - calcX;
            } else {
                return FADE_WIDTH - calcX;
            }
        }
    }
    return 0;
}

void changeBlending(bool blending) {
    // If changing blending to the same state, do nothing a state change its not worth it
    if (blending == blendingState) return;

    if (blending) {
        C2D_Flush();
        C3D_AlphaBlend(
            GPU_BLEND_ADD, GPU_BLEND_ADD,
            GPU_SRC_ALPHA, GPU_ONE,
            GPU_ONE, GPU_ZERO
        );

        C2D_Prepare();
        C3D_TexEnv *env = C3D_GetTexEnv(4);
        C3D_TexEnvInit(env);
        C3D_TexEnvSrc(env, C3D_Alpha, GPU_PREVIOUS, GPU_PREVIOUS, (GPU_TEVSRC)0);
        C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
    } else {
        C2D_Flush();
        C3D_AlphaBlend(
            GPU_BLEND_ADD, GPU_BLEND_ADD, 
            GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, 
            GPU_ONE, GPU_ZERO);
        
        C2D_Prepare();
        C3D_TexEnv *env = C3D_GetTexEnv(4);
        C3D_TexEnvInit(env);
    }

    blendingState = blending;
}

void drawBackground(float x, float y) {
    C2D_ImageTint tint = { 0 };

    Color col = channels[getColChannelIndex(static_cast<int>(ColorChannelIDs::ChannelBg))].color;

    // If flash is happening, use lbg
    if (state.flashData.useLbg) col = channels[getColChannelIndex(static_cast<int>(ColorChannelIDs::ChannelLbgNolerp))].color;

    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    float offset = 512 * BACKGROUND_SCALE;

    float calcX = positiveFmodf(x, offset);
    float drawY = -y;

    int bgID = levelInfo.backgroundID;
    int bgIdx = bgID & 0b11;

    // guard against an out-of-range sprite index for the loaded sheet
    if (bgSheet == NULL || (size_t)bgIdx >= C2D_SpriteSheetCount(bgSheet)) return;

    for (int i = -1; i < 3; i++) {
        C2D_Sprite bg = { 0 };
        // Calculate position for each tile
        float drawX = -calcX + i * offset;

        
        C2D_SpriteFromSheet(&bg, bgSheet, bgIdx);
        C3D_TexSetFilter(bg.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetPos(&bg, (int)drawX, (int)drawY);
        C2D_SpriteSetScale(&bg, BACKGROUND_SCALE, BACKGROUND_SCALE);
        C2D_DrawSpriteTinted(&bg, &tint);
    }
}

const int groundIndexes[G_COUNT] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26};

void drawGround(float camX, float camY, float y, bool isCeiling, int screenWidth) {
    changeBlending(false);
    int mult = (isCeiling ? -1 : 1);
    
    C2D_ImageTint tint = { 0 };
    Color col = channels[getColChannelIndex(static_cast<int>(ColorChannelIDs::ChannelGround))].color;
    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    int groundID = groundIndexes[levelInfo.groundID];
    bool hasL2 = levelInfo.groundID >= 7;

    C2D_Sprite ground = { 0 };
    C2D_SpriteFromSheet(&ground, groundSheet, groundID);
    C3D_TexSetFilter(ground.image.tex, GPU_LINEAR, GPU_LINEAR);

    if (isCeiling) y += GROUND_SIZE;

    float gY = y;

    if (hasL2) {
        if (!isCeiling) {
            gY -= GROUND_SIZE - ground.image.subtex->height - 1;
        } else {
            gY -= 1;
        }
    }

    // First draw the ground
    float calcX = 0 - positiveFmodf(camX, GROUND_SIZE);
    float calcY = SCREEN_HEIGHT - ((y - camY));
    float groundCalcY = SCREEN_HEIGHT - ((gY - camY));

    for (float i = -GROUND_SIZE; i < (screenWidth / SCALE) + GROUND_SIZE; i += GROUND_SIZE) {
        C2D_SpriteSetPos(&ground, calcX + i, groundCalcY);
        C2D_SpriteSetScale(&ground, 1.f, mult);
        C2D_DrawSpriteTinted(&ground, &tint);
    }

    if (hasL2) {
        col = channels[getColChannelIndex(static_cast<int>(ColorChannelIDs::ChannelGround2))].color;
        C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

        C2D_Sprite ground2 = { 0 };
        C2D_SpriteFromSheet(&ground2, groundSheet, groundID + 1);
        C3D_TexSetFilter(ground2.image.tex, GPU_LINEAR, GPU_LINEAR);

        float g2Y = y;
        if (isCeiling) g2Y -= GROUND_SIZE - ground2.image.subtex->height;

        groundCalcY = SCREEN_HEIGHT - ((g2Y - camY));
        for (float i = -GROUND_SIZE; i < (screenWidth / SCALE) + GROUND_SIZE; i += GROUND_SIZE) {
            C2D_SpriteSetPos(&ground2, calcX + i, groundCalcY);
            C2D_SpriteSetScale(&ground2, 1.f, mult);
            C2D_DrawSpriteTinted(&ground2, &tint);
        }
    }

    C2D_PlainImageTint(&tint, C2D_Color32(0, 0, 0, 100), 1.f);
    C2D_Sprite groundShadow = { 0 };

    C2D_SpriteFromSheet(&groundShadow, uiSheet, 361);
    C3D_TexSetFilter(groundShadow.image.tex, GPU_LINEAR, GPU_LINEAR);

    // Left shadow
    C2D_SpriteSetPos(&groundShadow, 0, calcY);
    C2D_SpriteSetScale(&groundShadow, 1.f, 1.f);
    C2D_DrawSpriteTinted(&groundShadow, &tint);

    // Right shadow
    C2D_SpriteSetPos(&groundShadow, screenWidth / SCALE, calcY);
    C2D_SpriteSetCenter(&groundShadow, 1.f, 0.f);
    C2D_SpriteSetScale(&groundShadow, -1.f, 1.f);
    C2D_DrawSpriteTinted(&groundShadow, &tint);

    int lineChan = getColChannelIndex(static_cast<int>(ColorChannelIDs::ChannelLine));
    // Then draw the line
    if (channels[lineChan].blending) {
        changeBlending(true);
    }

    col = channels[lineChan].color;
    C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

    float lineOffset = -((GROUND_SIZE / 2) - (LINE_HEIGHT / 2)) * mult;
    C2D_Sprite line = { 0 };
    C2D_SpriteFromSheet(&line, groundSheet, 0);
    C3D_TexSetFilter(line.image.tex, GPU_LINEAR, GPU_LINEAR);
    C2D_SpriteSetCenter(&line, 0.5f, 0.5f);
    C2D_SpriteSetPos(&line, screenWidth / SCALE / 2, (GROUND_SIZE / 2) + calcY + lineOffset);
    C2D_DrawSpriteTinted(&line, &tint);

    if (channels[lineChan].blending) {
        changeBlending(false);
    }
}

float completeTextElapsed = 0;
void drawEndWall(float delta) {  
    float calcX = ((levelInfo.wallX - state.cameraX));
    float calcY =  positiveFmodf(state.cameraY, 30) + 15;  
    if (levelInfo.wallY > 0) {
        // Draw each wall block
        for (float i = -30; i < SCREEN_HEIGHT_AREA + 30; i += 30) {
            C2D_Sprite block = { 0 };
            C2D_SpriteFromSheet(&block, spriteSheet, gameObjects[2].texture);
            C3D_TexSetFilter(block.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&block, 0.5f, 0.5f);
            C2D_SpriteSetPos(&block, getMirrorX(calcX, state.mirrorFactor), calcY + i);
            C2D_SpriteSetRotationDegrees(&block, adjustAngle(270, 0, state.mirrorMult < 0));
            C2D_DrawSprite(&block);
        }

        changeBlending(true);

        C2D_ImageTint tint = { 0 };
        Color col = getP2IfBlack(p1Color);
        C2D_PlainImageTint(&tint, C2D_Color32(col.r, col.g, col.b, 255), 1.f);

        // Draw glow
        for (float i = -30; i < SCREEN_HEIGHT_AREA + 30; i += 30) {
            C2D_Sprite glow = { 0 };
            C2D_SpriteFromSheet(&glow, spriteSheet, gameObjects[503].texture);
            C3D_TexSetFilter(glow.image.tex, GPU_LINEAR, GPU_LINEAR);
            C2D_SpriteSetCenter(&glow, 0.5f, 0.5f);
            C2D_SpriteSetPos(&glow, getMirrorX(calcX - 25, state.mirrorFactor), calcY + i);
            C2D_SpriteSetRotationDegrees(&glow, adjustAngle(270, 0, state.mirrorMult < 0));
            C2D_DrawSpriteTinted(&glow, &tint);
        }
    }   
    changeBlending(false);
}

void drawAttemptText() {
    int attempts = state.currentData.attempts;

    float calcX = (state.attemptTextPos.x - state.cameraX);
    float calcY = SCREEN_HEIGHT - ((state.attemptTextPos.y - state.cameraY));  

    if (calcX > -200) {
        C2D_SpriteSheet *fontSheet = levelFontSheet ? &levelFontSheet : &bigFontSheet;
        drawText(levelFont, fontSheet, getMirrorX(calcX, state.mirrorFactor), calcY, 1, (settingsState.doNot ? -1 : 1), 0.5f, true, "Attempt %d", attempts);
    }
}

static int getMargin(int id) {
    switch (id) {
        case 925:
        case 926:
            return 120;
    }
    return 60;
}


int compareIds(const void *a, const void *b) {
    return *(int *)a - *(int *)b;
}

static void loadCurrentObject(void) {
    currentObjectCount = 0;

    int width = ceilf(SCREEN_WIDTH_AREA / SECTION_SIZE);
    int height = ceilf(SCREEN_HEIGHT_AREA / SECTION_SIZE);
    int camSx = (int)(state.cameraX / SECTION_SIZE);
    int camSy = (int)((state.cameraY - LEVEL_Y_OFFSET) / SECTION_SIZE);

    for (int x = -2; x <= width + 1; x++) {
        for (int y = -2; y <= height + 1; y++) {
            int sx = camSx + x;
            int sy = camSy + y;

            Section *sec = getSection(sx, sy);
            for (int i = 0; i < sec->objectCount; i++) {
                int obj = sec->objects[i];

                float calcX = objects.x[obj] - state.cameraX;
                float calcY = SCREEN_HEIGHT - (objects.y[obj] - state.cameraY);
                
                int margin = getMargin(objects.id[obj]);

                float xMargin = margin * objects.scaleX[obj];
                float yMargin = margin * objects.scaleY[obj];

                if (calcX < -xMargin || calcX >= SCREEN_WIDTH / SCALE + xMargin || calcY < -yMargin || calcY >= SCREEN_HEIGHT / SCALE + yMargin) 
                    continue;

                if (!isValidObject(objects.id[obj]) || objects.flags[obj] & static_cast<int>(ObjectFlags::FlagToggled))
                    continue;

                // 0 scale objects are invisible
                if (objects.scaleX[obj] == 0.f || objects.scaleY[obj] == 0.f)
                    continue;
                
                if (currentObjectCount < MAX_SPRITES) currentObjects[currentObjectCount++] = obj;
            }
        }
    }

    // Sort it
    qsort(currentObjects, currentObjectCount, sizeof(int), compareIds);
}

static void updateCurrentObjects(void) {
    for (int i = 0; i < currentObjectCount; i++) {
        int obj = currentObjects[i];

        if (objects.alphaTriggerOpacity[obj] <= 0) {
            continue;
        }

        float calcX = objects.x[obj] - state.cameraX;
        float calcY = SCREEN_HEIGHT - (objects.y[obj] - state.cameraY);

        float fadeVal = objEdgeFade(calcX, GAMEPLAY_SCREEN_WIDTH_AREA);
        if (fadeVal == 255) {
            objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeNone);
        } else if (objects.transitionApplied[obj] == static_cast<int>(FadingEffects::FadeNone)) {
            float calcY = SCREEN_HEIGHT - (objects.y[obj] - state.cameraY);
            handleSpecialFading(obj, calcX, calcY);
        }
        
        // The rotating objects need to be recalculated
        float rotationSpeed = getRotationSpeed(obj);
        if (rotationSpeed != 0) {
            objects.visualRotation[obj] += ((objects.random[obj] & 1) ? -rotationSpeed : rotationSpeed) * delta;
        }

        spawnObjectParticles(obj);
        
        if (fadeVal == 255) {
            objects.transitionApplied[obj] = static_cast<int>(FadingEffects::FadeNone);
        }

        // Fade when it reaches an edge
        if (fadeVal != 255 && objects.transitionApplied[obj] == static_cast<int>(FadingEffects::FadeNone)) {
            handleSpecialFading(obj, calcX, calcY);
        }
        
        float fadeX = 0;
        float fadeY = 0;
        float fadeScale = 1.f;
        getFadeVars(obj, calcX, &fadeX, &fadeY, &fadeScale);

        // Handle special fade types
        fadeX += getSpecialFadingVars(obj, fadeVal);

        spawnObjectAt(
            obj,
            objects.id[obj],
            getMirrorX(calcX + fadeX, state.mirrorFactor),
            calcY + fadeY,
            objects.visualRotation[obj],
            objects.flippedH[obj] ^ (state.mirrorMult < 0),
            objects.flippedV[obj],
            fadeScale
        );

        spawnObjectParticles(obj);
    }
}

void updateTints() {
    u64 start = svcGetSystemTick();
    for (size_t s = 0; s < spriteCount; s++) {
        SpriteObject *obj = viewableObjectsPtr[s];
        if (obj->obj != -1) {
            int colChannel = obj->colChannel;

            ColorChannel col;

            if (colChannel == static_cast<int>(ColorChannelIDs::ChannelInvisibleGlow)) { // Handle invisible blocks color lerping
                int chan = getColChannelIndex(static_cast<int>(ColorChannelIDs::ChannelLbgNolerp));
                col.alpha = channels[chan].alpha;

                Color lbg = channels[chan].color;
                Color p1 = getWhiteIfBlack(p1Color);
                float opacity = objects.opacity[obj->obj];

                if (opacity < 0.8f || state.dead) {
                    col = channels[chan];
                } else {
                    float blendFactor = 1.9f - 1.5f * opacity;
                    float oneMinusFactor = 1.0f - blendFactor;

                    int r = (float)p1.r * oneMinusFactor + (float)lbg.r * blendFactor;
                    int g = (float)p1.g * oneMinusFactor + (float)lbg.g * blendFactor;
                    int b = (float)p1.b * oneMinusFactor + (float)lbg.b * blendFactor;
                    
                    col.color.r = CLAMP(r, 0, 255);
                    col.color.g = CLAMP(g, 0, 255);
                    col.color.b = CLAMP(b, 0, 255);
                    col.blending = true;
                }
            } else if (colChannel == static_cast<int>(ColorChannelIDs::ChannelLighter)) {
                // LIGHTER: derive the detail color from the object's main channel + lighter_hsv
                int mainCh = objects.colChannel[obj->obj];
                if (mainCh == 0 || mainCh == static_cast<int>(ColorChannelIDs::ChannelLighter))
                    mainCh = gameObjects[objects.id[obj->obj]].baseColor;
                mainCh = getColChannelIndex(mainCh);
                col = channels[mainCh];
                col.color = hsvCombine(col.color, lighterHSV);
                if (objects.mainColHSVEnabled[obj->obj]) {
                    col.color = hsvCombine(col.color, objects.mainColHSV[obj->obj]);
                }
                col.blending = false;
                // TODO: pulse interaction
            } else {
                col = channels[getColChannelIndex(colChannel)];
            }
            
            
            int gameObject = obj->obj;

            // Apply hsv
            if (obj->colType != static_cast<int>(ColorType::ColorTypeDetail)) {
                if (objects.mainColHSVEnabled[gameObject]) {
                    col.color = applyHSVToColor(col.color, gameObject, true, colChannel);
                }
            } else {
                if (objects.detailColHSVEnabled[gameObject]) {
                    col.color = applyHSVToColor(col.color, gameObject, false, colChannel);
                }
            }

            switch (obj->colType) {
                case static_cast<int>(ColorType::ColorTypeGlow):
                    col.blending = true;
                    break;
                case static_cast<int>(ColorType::ColorTypeWhite):
                    if (levelHasUnverifiedCoins() && objects.id[gameObject] == SECRET_COIN) {
                        col.color = (Color) {USER_COIN_UNRATED_R, USER_COIN_UNRATED_G, USER_COIN_UNRATED_B};
                    } else {
                        col.color = (Color) {255,255,255};
                    }
                    break;
            }

            bool detail = obj->colType == static_cast<int>(ColorType::ColorTypeDetail) || obj->colType == static_cast<int>(ColorType::ColorTypeOnlyDetail);

            if (!detail) {
                objects.mainNonPulseColor[gameObject] = col.color;
                if (objects.numMainPulses[gameObject] == 0) {
                    objects.mainColor[gameObject] = col.color;
                }
            } else {
                objects.detailNonPulseColor[gameObject] = col.color;
                if (objects.numDetailPulses[gameObject] == 0) {
                    objects.detailColor[gameObject] = col.color;
                }
            }   

            if (!detail && objects.mainBeingPulsed[gameObject] && colChannel >= 0) {
                col.color = objects.mainColor[gameObject];
            } else if (detail && objects.detailBeingPulsed[gameObject] && colChannel >= 0) {
                col.color = objects.detailColor[gameObject];
                if (colChannel == static_cast<int>(ColorChannelIDs::ChannelLighter) && objects.mainBeingPulsed[gameObject]) {
                    col.color = hsvCombine(col.color, lighterHSV);
                    if (objects.mainColHSVEnabled[gameObject]) {
                        col.color = applyHSVToColor(col.color, gameObject, true, colChannel);
                    }
                    col.blending = false;
                }
            }
            
            if (obj->colType == static_cast<int>(ColorType::ColorTypeBlack)) {
                col.color = (Color) {0,0,0};
                
                // Rod base ignore blending
                int objID = objects.id[obj->obj];
                if (objID >= 15 && objID <= 17) {
                    col.blending = false;
                }
            }

            float x = ((objects.x[gameObject] - state.cameraX));
            
            float opacity = obj->opacity;

            // Handle invisible object opacity
            if (objectFades(gameObject)) {
                float glowOut = 1.f;
                float fadingOpacity = getFadingObjFade(gameObject, GAMEPLAY_SCREEN_WIDTH_AREA, &glowOut);
                
                // Check if layer is glow
                if (obj->layer == 1) opacity *= glowOut;
                else opacity *= fadingOpacity;
            }

            int realOpacity = getObjOpacity(gameObject, x) * opacity * col.alpha * objects.alphaTriggerOpacity[gameObject];

            // Set opacity here
            objects.opacity[gameObject] = realOpacity / 255.f;

            obj->blending = col.blending;
            obj->hidden = (realOpacity == 0) || (col.blending && (col.color.r | col.color.g | col.color.b) == 0);
            
            obj->tint = C2D_Color32(col.color.r, col.color.g, col.color.b, realOpacity);
        }
    }
    
    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    snapshot.tintMs = ticks / CPU_TICKS_PER_MSEC;
}


void createObjects() {
    u64 start = svcGetSystemTick();

    spriteCount = 0;

    // Player sprite
    // Only needs one as its only for sorting purposes
    SpriteObject *vo = &viewableObjects[spriteCount];

    C2D_Sprite spr = { 0 };
    vo->spr = spr;
    vo->obj = -1;
    vo->layer = 0;
    vo->colType = 0;
    vo->opacity = 1.f;
    vo->colChannel = 0;
    viewableObjectsPtr[spriteCount] = vo;
    spriteCount++;

    loadCurrentObject();

    updateCurrentObjects();
    
    snapshot.drawCount = currentObjectCount;
    
    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    snapshot.creatingMs = ticks / CPU_TICKS_PER_MSEC;
    
    start = svcGetSystemTick();
    sortViewableObjects(viewableObjectsPtr, spriteCount);
    end = svcGetSystemTick();
    ticks = end - start;
    snapshot.sortingMs = ticks / CPU_TICKS_PER_MSEC;
    
    updateTints();
}

#define MAX_DASH_FX 8

struct DashEffect {
    bool active;
    bool exit;
    float x, y;
    float rotation;
    float elapsed;
    float scaleX, scaleY;
    Color color;
    int startFrame;
};

static DashEffect dashEffects[MAX_DASH_FX];

static const signed char dashFireOffX[12] = { -21, -22, -22, -24, -24, -24, -22, -21, -23, -23, -21, -19 };
static const signed char dashFireOffY[12] = { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const signed char dashOutlineOffX[12] = { -21, -22, -22, -24, -24, -24, -22, -21, -23, -22, -21, -19 };
static const signed char dashBoomOffX[9] = { -2, -1, -1, -1, -1, -1, 0, 0, 0 };
static const signed char dashBoomOffY[9] = { 1, 2, 3, 2, 2, 1, 1, 1, 1 };

static float dashEaseInOut(float t) {
    if (t < 0.5f) return 0.5f * (t * 2.f) * (t * 2.f);
    return 1.f - 0.5f * (2.f - t * 2.f) * (2.f - t * 2.f);
}

static void dashSpritePos(float x, float y, float rotation, float offX, float offY, float *outX, float *outY) {
    float rad = C3D_AngleFromDegrees(rotation);
    float cosR = cosf(rad);
    float sinR = sinf(rad);

    *outX = x + offX * cosR + offY * sinR;
    *outY = y + offX * sinR - offY * cosR;
}

static void dashFireState(const Player *player, float *localX, float *localY, float *sx, float *sy, Color *color) {
    float baseX = 0.9f;

    *localX = 0.f;
    *localY = 0.f;

    switch (player->gamemode) {
        case static_cast<int>(IconGamemode::GamemodeShip):
            *localY = 2.f;
            baseX = 1.0f;
            break;
        case static_cast<int>(IconGamemode::GamemodeUfo):
            baseX = 0.95f;
            break;
        case static_cast<int>(IconGamemode::GamemodeRobot):
            *localX = 1.f;
            break;
    }

    float finalScale = baseX * 1.1f;
    float t = player->dashFireTimer;

    if (t < 0.1f) {
        float k = dashEaseInOut(t / 0.1f);
        *sx = 0.3f + (finalScale * 0.9f - 0.3f) * k;
        *sy = 0.2f + (finalScale * 0.9f - 0.2f) * k;
    } else if (t < 0.2f) {
        float k = dashEaseInOut((t - 0.1f) / 0.1f);
        *sx = finalScale * 0.9f + (finalScale * 0.1f) * k;
        *sy = *sx;
    } else {
        *sx = finalScale;
        *sy = finalScale;
    }

    float ck = CLAMP((t - 0.1f) / 0.2f, 0.f, 1.f);
    Color target = (player == &state.player2) ? player->playerIcons.p1Color : player->playerIcons.p2Color;
    color->r = (unsigned char)(255.f + ((float)target.r - 255.f) * ck);
    color->g = (unsigned char)(255.f + ((float)target.g - 255.f) * ck);
    color->b = (unsigned char)(255.f + ((float)target.b - 255.f) * ck);
}

void drawDashFire(Player *player) {
    if (!player->dashing || state.dead || !playerEffectsSheet) return;

    float localX, localY, sx, sy;
    Color color;
    dashFireState(player, &localX, &localY, &sx, &sy, &color);

    float calcX = player->x - state.cameraX;
    float calcY = SCREEN_HEIGHT - (player->y - state.cameraY);

    float rotation = -player->dashAngle;
    float rad = C3D_AngleFromDegrees(rotation);
    float rotX = localX * cosf(rad) + localY * sinf(rad);
    float rotY = localX * sinf(rad) - localY * cosf(rad);

    float baseX = getMirrorX(calcX + rotX, state.mirrorFactor);
    float baseY = calcY + rotY;

    int frame = (int)(player->timeElapsed / ((0.04f / playerSpeedMults[state.speed]) * (1.f - fabsf(player->dashAngle) / 70.f * 0.4f))) % 12;

    C2D_ImageTint tint = { 0 };
    C2D_PlainImageTint(&tint, C2D_Color32(color.r, color.g, color.b, 255), 1.f);

    C2D_ImageTint outlineTint = { 0 };
    C2D_PlainImageTint(&outlineTint, C2D_Color32(255, 255, 255, 150), 1.f);

    float px, py;

    changeBlending(true);

    C2D_Sprite spr = { 0 };
    C2D_SpriteFromSheet(&spr, playerEffectsSheet, PLAYER_EFFECTS_FIRE_START + frame);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    dashSpritePos(baseX, baseY, rotation, dashFireOffX[frame], dashFireOffY[frame], &px, &py);
    C2D_SpriteSetPos(&spr, (int)px, (int)py);
    C2D_SpriteSetScale(&spr, sx, sy);
    C2D_SpriteSetRotationDegrees(&spr, rotation);
    C2D_DrawSpriteTinted(&spr, &tint);

    changeBlending(false);

    C2D_SpriteFromSheet(&spr, playerEffectsSheet, PLAYER_EFFECTS_OUTLINE_START + frame);
    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
    dashSpritePos(baseX, baseY, rotation, dashOutlineOffX[frame], 0.f, &px, &py);
    C2D_SpriteSetPos(&spr, (int)px, (int)py);
    C2D_SpriteSetScale(&spr, sx, sy);
    C2D_SpriteSetRotationDegrees(&spr, rotation);
    C2D_DrawSpriteTinted(&spr, &outlineTint);
}

static DashEffect *dashEffectFree(void) {
    for (int i = 0; i < MAX_DASH_FX; i++) {
        if (!dashEffects[i].active) return &dashEffects[i];
    }
    return NULL;
}

void addDashBoom(float x, float y, float rotation) {
    DashEffect *fx = dashEffectFree();
    if (!fx) return;

    fx->active = true;
    fx->exit = false;
    fx->x = x;
    fx->y = y;
    fx->rotation = rotation;
    fx->elapsed = 0.f;
    fx->scaleX = 1.2f;
    fx->scaleY = 1.2f;
    fx->color = (Color){ 0, 255, 0 };
    fx->startFrame = state.player.frame;
}

void addGravityDashBoom(float x, float y, float rotation) {
    DashEffect *fx = dashEffectFree();
    if (!fx) return;

    fx->active = true;
    fx->exit = false;
    fx->x = x;
    fx->y = y;
    fx->rotation = rotation;
    fx->elapsed = 0.f;
    fx->scaleX = 1.2f;
    fx->scaleY = 1.2f;
    fx->color = (Color){ 255, 0, 255 };
    fx->startFrame = state.player.frame;
}

void addDashExit(Player *player) {
    DashEffect *fx = dashEffectFree();
    if (!fx) return;

    float localX, localY, sx, sy;
    Color color;
    dashFireState(player, &localX, &localY, &sx, &sy, &color);

    float playerScale = player->mini ? 0.6f : 1.f;

    fx->active = true;
    fx->exit = true;
    fx->x = player->x + localX;
    fx->y = player->y + localY;
    fx->rotation = -player->dashAngle;
    fx->elapsed = 0.f;
    fx->scaleX = sx * playerScale;
    fx->scaleY = sy * playerScale;
    fx->color = color;
    fx->startFrame = state.player.frame;
}

void updateDashBooms(float delta) {
    for (int i = 0; i < MAX_DASH_FX; i++) {
        DashEffect *fx = &dashEffects[i];
        if (!fx->active) continue;

        fx->elapsed += delta;
        if (fx->elapsed >= (fx->exit ? 0.2f : 0.45f)) fx->active = false;
    }
}

void drawDashBooms(void) {
    static unsigned int lastFrame = 0;

    if (!playerEffectsSheet) return;

    if (frameCounter != lastFrame) {
        updateDashBooms(delta);
        lastFrame = frameCounter;
    }

    for (int i = 0; i < MAX_DASH_FX; i++) {
        DashEffect *fx = &dashEffects[i];
        if (!fx->active) continue;

        if (state.player.frame < fx->startFrame) {
            fx->active = false;
            continue;
        }

        float x = getMirrorX(fx->x - state.cameraX, state.mirrorFactor);
        float y = SCREEN_HEIGHT - (fx->y - state.cameraY);
        float sx, sy;
        float alpha;
        int frame;

        C2D_ImageTint tint = { 0 };

        if (fx->exit) {
            float k = fx->elapsed / 0.2f;
            frame = 0;
            sx = fx->scaleX * (1.f - 0.8f * k);
            sy = fx->scaleY * (1.f - 0.8f * k);
            alpha = 1.f - k;
            x -= 20.f * k;

            C2D_PlainImageTint(&tint, C2D_Color32(fx->color.r, fx->color.g, fx->color.b, (int)(alpha * 255.f)), 1.f);

            C2D_Sprite spr = { 0 };
            float px, py;

            C2D_SpriteFromSheet(&spr, playerEffectsSheet, PLAYER_EFFECTS_FIRE_START + frame);
            C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
            dashSpritePos(x, y, fx->rotation, dashFireOffX[frame], dashFireOffY[frame], &px, &py);
            C2D_SpriteSetPos(&spr, (int)px, (int)py);
            C2D_SpriteSetScale(&spr, sx, sy);
            C2D_SpriteSetRotationDegrees(&spr, fx->rotation);
            C2D_DrawSpriteTinted(&spr, &tint);

            changeBlending(false);

            C2D_SpriteFromSheet(&spr, playerEffectsSheet, PLAYER_EFFECTS_OUTLINE_START + frame);
            C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
            dashSpritePos(x, y, fx->rotation, dashOutlineOffX[frame], 0.f, &px, &py);
            C2D_SpriteSetPos(&spr, (int)px, (int)py);
            C2D_SpriteSetScale(&spr, sx, sy);
            C2D_SpriteSetRotationDegrees(&spr, fx->rotation);
            C2D_PlainImageTint(&tint, C2D_Color32(255, 255, 255, (int)(alpha * 150.f)), 1.f);
            C2D_DrawSpriteTinted(&spr, &tint);

            changeBlending(true);
        } else {
            frame = (int)(fx->elapsed / 0.05f);
            if (frame > 8) frame = 8;
            sx = fx->scaleX;
            sy = fx->scaleY;
            alpha = (fx->elapsed <= 0.2f) ? 1.f : 1.f - (fx->elapsed - 0.2f) / 0.3f;

            float ck = CLAMP(fx->elapsed / 0.3f, 0.f, 1.f);
            unsigned char cr = (unsigned char)(255.f + ((float)fx->color.r - 255.f) * ck);
            unsigned char cg = (unsigned char)(255.f + ((float)fx->color.g - 255.f) * ck);
            unsigned char cb = (unsigned char)(255.f + ((float)fx->color.b - 255.f) * ck);

            C2D_PlainImageTint(&tint, C2D_Color32(cr, cg, cb, 255), alpha);

            C2D_Sprite spr = { 0 };
            float px, py;

            C2D_SpriteFromSheet(&spr, playerEffectsSheet, PLAYER_EFFECTS_BOOM_START + frame);
            C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
            dashSpritePos(x, y, fx->rotation, dashBoomOffX[frame], dashBoomOffY[frame], &px, &py);
            C2D_SpriteSetPos(&spr, (int)px, (int)py);
            C2D_SpriteSetScale(&spr, sx, sy);
            C2D_SpriteSetRotationDegrees(&spr, fx->rotation);
            C2D_DrawSpriteTinted(&spr, &tint);
        }
    }
}

#define MAX_SPIDER_DASH_FX 4
#define SPIDER_DASH_FRAME_TIME 0.04f
#define SPIDER_DASH_FRAME_COUNT 8
#define SPIDER_DASH_LIFETIME (SPIDER_DASH_FRAME_TIME * SPIDER_DASH_FRAME_COUNT)
#define SPIDER_DASH_WIDTH 221.f

struct SpiderDashEffect {
    bool active;
    float x, y;
    float rotation;
    float scaleX;
    float elapsed;
    Color color;
    int startFrame;
};

static SpiderDashEffect spiderDashEffects[MAX_SPIDER_DASH_FX];

static const signed char spiderDashOffX[SPIDER_DASH_FRAME_COUNT] = { 2, -4, -31, -47, -55, -72, -79, -98 };
static const signed char spiderDashOffY[SPIDER_DASH_FRAME_COUNT] = { 0, 0, 0, 1, 1, 1, 1, 2 };

void addSpiderDashEffect(float x, float fromY, float toY, bool upsideDown, Color color) {
    SpiderDashEffect *fx = NULL;

    for (int i = 0; i < MAX_SPIDER_DASH_FX; i++) {
        if (!spiderDashEffects[i].active) {
            fx = &spiderDashEffects[i];
            break;
        }
    }

    if (!fx) return;

    fx->active = true;
    fx->x = x;
    fx->y = fromY + (toY - fromY) * 0.5f;
    fx->rotation = upsideDown ? 90.f : -90.f;
    fx->scaleX = (fabsf(fromY - toY) + 30.f) / SPIDER_DASH_WIDTH;
    fx->elapsed = 0.f;
    fx->color = color;
    fx->startFrame = state.player.frame;
}

static void drawSpiderDashEffects(void) {
    static unsigned int lastFrame = 0;

    if (!playerEffectsSheet) return;

    if (frameCounter != lastFrame) {
        for (int i = 0; i < MAX_SPIDER_DASH_FX; i++) {
            SpiderDashEffect *fx = &spiderDashEffects[i];
            if (!fx->active) continue;

            fx->elapsed += 1.f / 60.f;
            if (fx->elapsed >= SPIDER_DASH_LIFETIME) fx->active = false;
        }
        lastFrame = frameCounter;
    }

    for (int i = 0; i < MAX_SPIDER_DASH_FX; i++) {
        SpiderDashEffect *fx = &spiderDashEffects[i];
        if (!fx->active) continue;

        if (state.player.frame < fx->startFrame) {
            fx->active = false;
            continue;
        }

        float x = getMirrorX(fx->x - state.cameraX, state.mirrorFactor);
        float y = SCREEN_HEIGHT - (fx->y - state.cameraY);

        int frame = (int)(fx->elapsed / SPIDER_DASH_FRAME_TIME);
        if (frame >= SPIDER_DASH_FRAME_COUNT) frame = SPIDER_DASH_FRAME_COUNT - 1;

        float px, py;
        dashSpritePos(x, y, fx->rotation, spiderDashOffX[frame] * fx->scaleX, spiderDashOffY[frame], &px, &py);

        C2D_ImageTint tint = { 0 };
        C2D_PlainImageTint(&tint, C2D_Color32(fx->color.r, fx->color.g, fx->color.b, 255), 1.f);

        C2D_Sprite spr = { 0 };
        C2D_SpriteFromSheet(&spr, playerEffectsSheet, PLAYER_EFFECTS_SPIDER_START + frame);
        C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
        C2D_SpriteSetPos(&spr, (int)px, (int)py);
        C2D_SpriteSetScale(&spr, fx->scaleX, 1.f);
        C2D_SpriteSetRotationDegrees(&spr, fx->rotation);
        C2D_DrawSpriteTinted(&spr, &tint);
    }
}

void drawPlayerEffects() {
    changeBlending(true);
    if (state.showPlayer) {
        for (int i = 0; i < 2; i++) {
            drawParticleSystem(&dragParticles[i], 0, 0, 1.f);
            drawParticleSystem(&shipFireParticles[i], 0, 0, 1.f);
            drawParticleSystem(&shipSecondaryParticles[i], 0, 0, 1.f);
            drawParticleSystem(&secondaryParticles[i], 0, 0, 1.f);
            drawParticleSystem(&burstParticles[i], 0, 0, 1.f);
            drawParticleSystem(&robotFireParticles[i], 0, 0, 1.f);
            drawParticleSystem(&robotFireParticles[i], 0, 0, 1.f);
            drawParticleSystem(&landParticles[i], 0, 0, 1.f);
            drawParticleSystem(&explosionParticles[i], 0, 0, 1.f);
        }
    }
    drawParticleSystem(&brickDestroyParticles, 0, 0, 1.f);
    drawParticleSystem(&coinPickupParticles, 0, 0, 1.f);
    drawParticleSystem(&glitterParticles, 0, 0, 1.f);
    drawP1Trail(&state.player, 0);
    if (!settingsState.noPlayerTrail) motionTrailDraw(&trailP1);
    motionTrailDrawWaveTrail(&waveTrailP1);
}

void drawPostPlayerEffects() {
    changeBlending(true);
    for (int i = 0; i < 2; i++) {
        drawParticleSystem(&dragParticles2[i], 0, 0, 1.f);
    }
    changeBlending(false);
}

void drawPlayerGraphics() {
    changeBlending(false);
    
    drawCollectEffect();
    drawCollectableEffect();

    changeBlending(true);
    drawUseEffects(getUseEffectArrayPtr(GFX_TOP));
    if (levelInfo.wallY > 0) {
        drawParticleSystem(&endWallParticles, 0, 0, 1);
        // Render rays
        drawRays();
    }
    drawObjectParticles();

    drawPlayerEffects();

    drawP1Trail(&state.player2, 1);
    
    if (!settingsState.noPlayerTrail) motionTrailDraw(&trailP2);
    motionTrailDrawWaveTrail(&waveTrailP2);

    drawAllLightning();

    changeBlending(false);
    state.currentPlayer = 0;

    if (state.showPlayer) {
        changeBlending(true);
        drawDashBooms();
        drawSpiderDashEffects();
        changeBlending(false);
    }

    drawCheckpoints();
    if (state.showPlayer) {
        drawDashFire(&state.player);
        drawPlayer(&state.player);
        
        if (state.dual) {
            state.currentPlayer = 1;
            drawDashFire(&state.player2);
            drawPlayer(&state.player2);
        }  
    }

    if (state.showPlayer) drawPostPlayerEffects();
}

void drawObjects() {
    u64 start = svcGetSystemTick();

    // Draw
    for (size_t s = 0; s < spriteCount; s++) {
        SpriteObject *obj = viewableObjectsPtr[s];

        if (obj->obj != -1) {
            if (obj->hidden) continue;

            changeBlending(obj->blending);

            if (isCollectableObject(objects.id[obj->obj])) {
                QuadParams keyParams = obj->params;
                float off = collectableFloatOffset();
                keyParams.quadr.topLeft[1]  += off;
                keyParams.quadr.topRight[1] += off;
                keyParams.quadr.botLeft[1]  += off;
                keyParams.quadr.botRight[1] += off;
                c2dDrawImageFast(&obj->spr.image, &keyParams, &obj->spr.params, obj->tint);
            } else {
                c2dDrawImageFast(&obj->spr.image, &obj->params, &obj->spr.params, obj->tint);
            }
        } else {   
            drawPlayerGraphics();
        }
    }

    changeBlending(true);
    drawParticleSystem(&slowSpeedParticles, 0, 0, 1.f);
    drawParticleSystem(&normalSpeedParticles, 0, 0, 1.f);
    drawParticleSystem(&fastSpeedParticles, 0, 0, 1.f);
    drawParticleSystem(&fasterSpeedParticles, 0, 0, 1.f);
    drawParticleSystem(&fastestSpeedParticles, 0, 0, 1.f);
    changeBlending(false);

    if (state.hitboxDisplay) {
        drawRotatedHitbox(&state.player);
        drawPlayerHitbox(&state.player);
        drawInternalHitbox(&state.player);
        if (state.hitboxDisplay == 2) drawHitboxTrail(0);

        if (state.dual) {
            drawRotatedHitbox(&state.player2);
            drawPlayerHitbox(&state.player2);
            drawInternalHitbox(&state.player2);
            if (state.hitboxDisplay == 2) drawHitboxTrail(1);
        }

        for (int i = 0; i < currentObjectCount; i++) {
            int obj = currentObjects[i];
            drawHitbox(obj);
        }
    }

    u64 end = svcGetSystemTick();
    u64 ticks = end - start;
    snapshot.drawingMs = ticks / CPU_TICKS_PER_MSEC;
    snapshot.renderingMs += ticks / CPU_TICKS_PER_MSEC;
}

void updateTouchEffect(float delta) {
    touchPosition pos;
    hidTouchRead(&pos);

    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    touchDragParticles.emitting = false;

    bool tapEffectEnabled = (gameState == static_cast<int>(GameState::StateGame) && !gamePaused) ? settingsState.gameplayTapEffect : settingsState.touchEffectEverywhere;

    if (tapEffectEnabled && (kHeld & KEY_TOUCH)) {
        // Flipped for particles
        float flippedY = SCREEN_HEIGHT - pos.py;

        // Use effect
        if (kDown & KEY_TOUCH) {
            UseEffect *effect = addUseEffect(pos.px, pos.py, USE_EFFECT_OBJ_NOTHING, &tapEffect, getUseEffectArrayPtr(GFX_BOTTOM));    
            touchExplosionParticles.emitterX = pos.px;
            touchExplosionParticles.emitterY = flippedY;
            spawnMultipleParticles(&touchExplosionParticles, 50);
            if (effect) {
                Color p1NotWhite = getWhiteIfBlack(p1Color);

                effect->def.colorR = p1NotWhite.r / 255.f;
                effect->def.colorG = p1NotWhite.g / 255.f;
                effect->def.colorB = p1NotWhite.b / 255.f;
            }
            touchEffectDragTimer = 0.08f;
        }

        touchDragParticles.emitterX = pos.px;
        touchDragParticles.emitterY = flippedY;
        touchDragParticles.emitting = (touchEffectDragTimer <= 0);
        if (touchEffectDragTimer > 0) {
            touchEffectDragTimer -= delta;
        }

        
    }
    updateUseEffects(delta, getUseEffectArrayPtr(GFX_BOTTOM));
    updateParticleSystem(&touchExplosionParticles, delta);
    updateParticleSystem(&touchDragParticles, delta);
}

void drawTouchEffect() {
    drawUseEffects(getUseEffectArrayPtr(GFX_BOTTOM));
    drawParticleSystem(&touchDragParticles, 0, 0, 1.f);
    drawParticleSystem(&touchExplosionParticles, 0, 0, 1.f);
}

void drawBottomParticles() {
    drawParticleSystem(&glitterParticlesBottom, 0, 0, 1.f);
    drawParticleSystem(&slowSpeedParticlesBottom, 0, 0, 1.f);
    drawParticleSystem(&normalSpeedParticlesBottom, 0, 0, 1.f);
    drawParticleSystem(&fastSpeedParticlesBottom, 0, 0, 1.f);
    drawParticleSystem(&fasterSpeedParticlesBottom, 0, 0, 1.f);
    drawParticleSystem(&fastestSpeedParticlesBottom, 0, 0, 1.f);
}

void updateBottomParticles(float delta) {
    glitterParticlesBottom.emitting = false;

    bool flyingGamemode = (state.player.gamemode == static_cast<int>(IconGamemode::GamemodeShip) || state.player.gamemode == static_cast<int>(IconGamemode::GamemodeUfo) || state.player.gamemode == static_cast<int>(IconGamemode::GamemodeWave));
    if (state.dual) flyingGamemode = flyingGamemode || (state.player2.gamemode == static_cast<int>(IconGamemode::GamemodeShip) || state.player2.gamemode == static_cast<int>(IconGamemode::GamemodeUfo) || state.player2.gamemode == static_cast<int>(IconGamemode::GamemodeWave));

    // If in game and not paused and not fading, update the particles spawning
    if (((gameState == static_cast<int>(GameState::StateGame) && !gamePaused))) {
        if (flyingGamemode) {
            glitterParticlesBottom.emitterX = state.cameraXMiddle;
            glitterParticlesBottom.emitterY = 240/2;
            glitterParticlesBottom.emitting = true;
        }
        slowSpeedParticlesBottom.emitting = slowSpeedParticlesTimer > 0;
        slowSpeedParticlesBottom.emitterX = 320/SCALE;
        slowSpeedParticlesBottom.emitterY = 240/2;

        normalSpeedParticlesBottom.emitting = normalSpeedParticlesTimer > 0;
        normalSpeedParticlesBottom.emitterX = 320/SCALE;
        normalSpeedParticlesBottom.emitterY = 240/2;

        fastSpeedParticlesBottom.emitting = fastSpeedParticlesTimer > 0;
        fastSpeedParticlesBottom.emitterX = 320/SCALE;
        fastSpeedParticlesBottom.emitterY = 240/2;
        
        fasterSpeedParticlesBottom.emitting = fasterSpeedParticlesTimer > 0;
        fasterSpeedParticlesBottom.emitterX = 320/SCALE;
        fasterSpeedParticlesBottom.emitterY = 240/2;

        fastestSpeedParticlesBottom.emitting = fastestSpeedParticlesTimer > 0;
        fastestSpeedParticlesBottom.emitterX = 320/SCALE;
        fastestSpeedParticlesBottom.emitterY = 240/2;
    }

    updateParticleSystem(&glitterParticlesBottom, delta);
    updateParticleSystem(&slowSpeedParticlesBottom, delta);
    updateParticleSystem(&normalSpeedParticlesBottom, delta);
    updateParticleSystem(&fastSpeedParticlesBottom, delta);
    updateParticleSystem(&fasterSpeedParticlesBottom, delta);
    updateParticleSystem(&fastestSpeedParticlesBottom, delta);
}

static int iconPartTexture(const IconPart *parts, int partIndex) {
    if (parts[partIndex].texture < 0) return -1;

    int texture = 0;
    for (int i = 0; i < partIndex; i++) {
        if (parts[i].texture >= 0) texture++;
    }
    return texture;
}


const PlayerAnimation *getGamemodeAnimation(int gamemode) {
    if (gamemode == static_cast<int>(IconGamemode::GamemodeRobot)) return robotAnimations;
    if (gamemode == static_cast<int>(IconGamemode::GamemodeSpider)) return spiderAnimations;
    return NULL;
}

int getAnimationCount(int gamemode) {
    if (gamemode == static_cast<int>(IconGamemode::GamemodeRobot)) return static_cast<int>(RobotAnimId::RobotAnimCount);
    if (gamemode == static_cast<int>(IconGamemode::GamemodeSpider)) return static_cast<int>(SpiderAnimId::SpiderAnimCount);
    return 0;
}

static void spawnIconAtInternal(
    int gamemode,
    int id,
    bool glow,
    bool drawWhite,
    float x,
    float y,
    float deg,
    unsigned char flipX,
    unsigned char flipY,
    float scale,
    u32 p1Color,
    u32 p2Color,
    u32 glowColor,
    C2D_SpriteSheet sheet,
    IconParameters params
) {
    const Icon icon = icons[gamemode][id];
    const IconPart *parts = icon.parts;

    int flipXMult = (flipX ? -1 : 1);
    int flipYMult = (flipY ? -1 : 1);

    float rad = C3D_AngleFromDegrees(deg) * flipXMult;
    float cosR = cosf(rad);
    float sinR = sinf(rad);

    float sx = scale * flipXMult;
    float sy = scale * flipYMult;

    C2D_Sprite spr = { 0 };

    size_t count = icon.partCount;

    if (icon.partCount < 2) return;

    C2D_ImageTint tints[4];

    if (drawWhite) {
        C2D_PlainImageTint(&tints[static_cast<int>(IconColorType::IconColorWhite)], C2D_Color32(255, 255, 255, 255), 1.0f);
    } else {
        C2D_PlainImageTint(&tints[static_cast<int>(IconColorType::IconColorWhite)], 0, 1.0f);
    }
    C2D_PlainImageTint(&tints[static_cast<int>(IconColorType::IconColorP1)], p1Color, 1.0f);
    C2D_PlainImageTint(&tints[static_cast<int>(IconColorType::IconColorP2)], p2Color, 1.0f);

    if (glow) {
        C2D_PlainImageTint(&tints[static_cast<int>(IconColorType::IconColorGlow)], glowColor, 1.0f);
    } else {
        C2D_PlainImageTint(&tints[static_cast<int>(IconColorType::IconColorGlow)], 0, 1.0f);
    }

    if (gamemode == static_cast<int>(IconGamemode::GamemodeRobot) || gamemode == static_cast<int>(IconGamemode::GamemodeSpider)) {
        const PlayerAnimation *animations = getGamemodeAnimation(gamemode);
        if (!animations) return;

        const PlayerAnimation *anim = &animations[params.animID];
        if (params.animFrame >= anim->frameCount)
            params.animFrame = 0;
        const PlayerFrame *frame = &anim->frames[params.animFrame];
        const PlayerFrame *nextFrame = frame;
        float animFraction = 0.0f;
        if (anim->frameCount > 1) {
            int nextIdx = params.animFrame + 1;
            if (nextIdx >= anim->frameCount) nextIdx = 0;
            bool holdLast = nextIdx == 0 && (params.animID == static_cast<int>(RobotAnimId::RobotAnimJumpStart)
                                            || params.animID == static_cast<int>(RobotAnimId::RobotAnimFallStart));
            if (!holdLast) {
                nextFrame = &anim->frames[nextIdx];
                animFraction = params.animTimer / frame->delay;
            }
        }

        const PlayerAnimation *blend_from_anim = NULL;
        const PlayerFrame *blendFromFrame = NULL;
        const PlayerFrame *blend_from_next_frame = NULL;
        float blendFromFraction = 0.f;
        if (params.blendFraction > 0.f && params.blendFromAnimID >= 0 && params.blendFromAnimID < getAnimationCount(gamemode)) {
            blend_from_anim = &animations[params.blendFromAnimID];

            if (params.blendFromFrame >= 0 && params.blendFromFrame < blend_from_anim->frameCount) {
                blendFromFrame = &blend_from_anim->frames[params.blendFromFrame];
                blend_from_next_frame = blendFromFrame;
                
                if (blend_from_anim->frameCount > 1) {
                    int nextIdx = params.blendFromFrame + 1;
                    if (nextIdx >= blend_from_anim->frameCount) nextIdx = 0;
                    bool holdLast = nextIdx == 0 && (params.blendFromAnimID == static_cast<int>(RobotAnimId::RobotAnimJumpStart)
                                                    || params.blendFromAnimID == static_cast<int>(RobotAnimId::RobotAnimFallStart));
                    if (!holdLast) {
                        blend_from_next_frame = &blend_from_anim->frames[nextIdx];
                        blendFromFraction = params.blendFromTimer / blendFromFrame->delay;
                    }
                }
                if (blendFromFrame->partCount != frame->partCount) {
                    blendFromFrame = NULL;
                }
            }
        }
        
        for (int i = 0; i < frame->partCount; i++) {
            const PlayerSpritePart *animationPart = &frame->parts[i];
            const PlayerSpritePart *nextPart = &nextFrame->parts[i];
            float animPx = animationPart->px + (nextPart->px - animationPart->px) * animFraction;
            float animPy = animationPart->py + (nextPart->py - animationPart->py) * animFraction;
            float animRot = nextPart->rotation - animationPart->rotation;
            if (animRot > 180.0f) animRot -= 360.0f;
            if (animRot < -180.0f) animRot += 360.0f;
            animRot = animationPart->rotation + animRot * animFraction;

            if (blendFromFrame) {
                const PlayerSpritePart *fromPart = &blendFromFrame->parts[i];
                const PlayerSpritePart *fromNextPart = &blend_from_next_frame->parts[i];
                float fromPx = fromPart->px + (fromNextPart->px - fromPart->px) * blendFromFraction;
                float fromPy = fromPart->py + (fromNextPart->py - fromPart->py) * blendFromFraction;
                float fromRot = fromNextPart->rotation - fromPart->rotation;
                if (fromRot > 180.0f) fromRot -= 360.0f;
                if (fromRot < -180.0f) fromRot += 360.0f;
                fromRot = fromPart->rotation + fromRot * blendFromFraction;

                float blend = params.blendFraction;
                animPx = fromPx + (animPx - fromPx) * blend;
                animPy = fromPy + (animPy - fromPy) * blend;
                float rotDelta = animRot - fromRot;
                if (rotDelta > 180.0f) rotDelta -= 360.0f;
                if (rotDelta < -180.0f) rotDelta += 360.0f;
                animRot = fromRot + rotDelta * blend;
            }

            int texturePart = animationPart->textureIdx / 2;
        
            size_t index;
            for (index = 0; index < count; index++) {
                const IconPart *part = &parts[index];
                if (part->animationPart - 1 == texturePart) {
                    break;
                }
            }
            
            if (index == count) continue;

            // Find layer count
            size_t layerCount = 0;
            while (index + layerCount < count) {
                const IconPart *testPart = &parts[index + layerCount];

                if (testPart->animationPart - 1 != texturePart)
                    break;

                layerCount++;
            }

            for (size_t j = 0; j < layerCount; j++) {
                int realIndex = j;

                // Swap p1 and p2
                if (j==0) realIndex = 1;
                else if (j==1) realIndex = 0;

                const IconPart *part = &parts[index + realIndex];

                int localTexture = iconPartTexture(parts, (int)(index + realIndex));
                if (localTexture >= 0) {
                    float partRad = rad + C3D_AngleFromDegrees(animRot) * flipXMult * flipYMult;
                    float partCosR = cosf(partRad);
                    float partSinR = sinf(partRad);

                    float animX = animPx * 2.0f * flipXMult;
                    float animY = animPy * 2.0f * flipYMult;

                    float partX = part->x * flipXMult;
                    float partY = part->y * flipYMult;

                    float rotAnimX = animX * cosR + animY * sinR;
                    float rotAnimY = animX * sinR - animY * cosR;

                    float rotPartX = partX * partCosR + partY * partSinR;
                    float rotPartY = partX * partSinR - partY * partCosR;

                    float pX = x + (rotAnimX + rotPartX) * scale;
                    float pY = y + (rotAnimY + rotPartY) * scale;

                    float partSx = (animationPart->scaleX < 0.0f) ? -sx : sx;
                    float partSy = (animationPart->scaleY < 0.0f) ? -sy : sy;

                    if (i == 6 && j == 0 && drawWhite && params.robotFireScale > 0.f && playerEffectsSheet) {
                        C2D_SpriteFromSheet(&spr, playerEffectsSheet, PLAYER_EFFECTS_FIRE_BOOST);
                        C2D_SpriteSetCenter(&spr, 0.5f, 0.0f);
                        C2D_SpriteSetPos(&spr, pX, pY);
                        C2D_SpriteSetScale(&spr, sx * params.robotFireScale, scale * params.robotFireScale);
                        C2D_SpriteSetRotation(&spr, partRad + C3D_AngleFromDegrees(180.0f * flipY));
                        C2D_DrawSpriteTinted(&spr, &tints[static_cast<int>(IconColorType::IconColorWhite)]);
                    }

                    C2D_SpriteFromSheet(&spr, sheet, localTexture);
                    C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
                    C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

                    C2D_SpriteSetPos(&spr, pX, pY);
                    C2D_SpriteSetScale(&spr, partSx, partSy);
                    C2D_SpriteSetRotation(&spr, partRad);

                    C2D_DrawSpriteTinted(&spr, &tints[static_cast<int>(part->colorType)]);
                }
            }
        }
    } else {
        for (size_t i = 0; i < count; i++) {
            size_t realIndex = i;
            // Swap p1 and p2 layers
            if (i==0) realIndex = 1;
            else if (i==1) realIndex = 0;

            if (gamemode == static_cast<int>(IconGamemode::GamemodeUfo)) {
                if (i==2) realIndex = 0;
                else if (i < 2) realIndex++;
            }
            
            const IconPart *part = &parts[realIndex];

            int localTexture = iconPartTexture(parts, (int) realIndex);
            if (localTexture >= 0) {

                float localX = part->x * flipXMult;
                float localY = part->y * flipYMult;

                float rotX = localX * cosR + localY * sinR;
                float rotY = localX * sinR - localY * cosR;

                float pX = x + rotX * scale;
                float pY = y + rotY * scale;

                C2D_SpriteFromSheet(&spr, sheet, localTexture);
                C2D_SpriteSetCenter(&spr, 0.5f, 0.5f);
                C3D_TexSetFilter(spr.image.tex, GPU_LINEAR, GPU_LINEAR);

                C2D_SpriteSetPos(&spr, pX, pY);
                C2D_SpriteSetScale(&spr, sx, sy);
                C2D_SpriteSetRotation(&spr, rad);

                C2D_DrawSpriteTinted(&spr, &tints[static_cast<int>(part->colorType)]);
            }
        }
    }
}

void spawnIconAt(
    int gamemode,
    int id,
    bool glow,
    float x,
    float y,
    float deg,
    unsigned char flipX,
    unsigned char flipY,
    float scale,
    u32 p1Color,
    u32 p2Color,
    u32 glowColor,
    IconParameters params
) {
    C2D_SpriteSheet sheet = iconCacheGet(gamemode, id);
    if (!sheet) return;

    if (glow) {
        spawnIconAtInternal(
            gamemode,
            id,
            true,
            false,
            x,
            y,
            deg,
            flipX,
            flipY,
            scale,
            0,
            0,
            glowColor,
            sheet,
            params
        );
    }

    spawnIconAtInternal(
        gamemode,
        id,
        false,
        true,
        x,
        y,
        deg,
        flipX,
        flipY,
        scale,
        p1Color,
        p2Color,
        0,
        sheet,
        params
    );
}

void spawnP1LayerAt(
    int gamemode,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flipX,
    unsigned char flipY,
    float scale,
    u32 p1Color,
    IconParameters params
) {
    C2D_SpriteSheet sheet = iconCacheGet(gamemode, id);
    if (!sheet) return;

    spawnIconAtInternal(
        gamemode,
        id,
        false,
        false,
        x,
        y,
        deg,
        flipX,
        flipY,
        scale,
        p1Color,
        0,
        0,
        sheet,
        params
    );
}

void spawnGlowLayerAt(
    int gamemode,
    int id,
    float x,
    float y,
    float deg,
    unsigned char flipX,
    unsigned char flipY,
    float scale,
    u32 glowColor,
    IconParameters params
) {
    C2D_SpriteSheet sheet = iconCacheGet(gamemode, id);
    if (!sheet) return;

    spawnIconAtInternal(
        gamemode,
        id,
        true,
        false,
        x,
        y,
        deg,
        flipX,
        flipY,
        scale,
        0,
        0,
        glowColor,
        sheet,
        params
    );
}

float approachf(float current, float target, float speed, float smoothing) {
    float diff = target - current;
    float step = diff * smoothing; // smoothing in [0,1], e.g. 0.1 for gentle, 0.5 for fast
    if (fabsf(diff) < speed)
        return target;
    return current + step + (diff > 0 ? speed : -speed);
}


void handleMirrorTransition() {
    if (state.mirroring) {
        // Do the easing
        state.mirrorFactor = easeValue(EaseTypes::EaseInOut, state.originalMirrorFactor, state.intendedMirrorFactor, state.mirrorTimer, MIRROR_DURATION, 1.2);

        state.mirrorSpeedFactor = 1 - 2*state.mirrorFactor;
        if (state.mirrorFactor >= 0.5f) {
            state.mirrorMult = -1;
        } else {
            state.mirrorMult = 1;
        }
    }
}

}
