#include <3ds.h>
#include <citro2d.h>
#include "main.hpp"

#include "menus/core/ui_screen.hpp"
#include "math_helpers.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_window.hpp"
#include "menus/components/ui_color_button.hpp"
#include "menus/components/ui_window_button.hpp"
#include "graphics.hpp"
#include "palette_kit.hpp"
#include "menus/components/ui_checkbox.hpp"

extern const u32 colors[] = {
    // Reds
    ABGR8(253, 212, 206, 255),
    ABGR8(255, 125, 125, 255),
    ABGR8(255, 58, 58, 255),
    ABGR8(255, 0, 0, 255),

    ABGR8(150, 0, 0, 255),
    ABGR8(112, 0, 0, 255),
    ABGR8(82, 2, 0, 255),
    ABGR8(56, 1, 6, 255),

    ABGR8(175, 0, 75, 255),
    ABGR8(128, 79, 79, 255),
    ABGR8(122, 53, 53, 255),
    ABGR8(81, 36, 36, 255),

    // Oranges
    ABGR8(255, 185, 114, 255),
    ABGR8(255, 160, 64, 255),
    ABGR8(255, 125, 0, 255),
    ABGR8(255, 75, 0, 255),

    ABGR8(175, 75, 0, 255),
    ABGR8(163, 98, 70, 255),
    ABGR8(117, 73, 54, 255),
    ABGR8(86, 53, 40, 255),

    ABGR8(150, 50, 0, 255),
    ABGR8(102, 49, 30, 255),
    ABGR8(91, 39, 0, 255),
    ABGR8(71, 32, 0, 255),

    // Yellows
    ABGR8(255, 255, 192, 255),
    ABGR8(255, 250, 127, 255),
    ABGR8(255, 255, 0, 255),
    ABGR8(125, 125, 0, 255),

    ABGR8(253, 224, 160, 255),
    ABGR8(255, 185, 0, 255),
    ABGR8(150, 100, 0, 255),
    ABGR8(80, 50, 14, 255),

    ABGR8(205, 165, 118, 255),
    ABGR8(167, 123, 77, 255),
    ABGR8(109, 83, 57, 255),
    ABGR8(81, 62, 42, 255),

    // Greens
    ABGR8(192, 255, 160, 255),
    ABGR8(177, 255, 109, 255),
    ABGR8(125, 255, 0, 255),
    ABGR8(0, 255, 0, 255), // Default p1

    ABGR8(210, 255, 50, 255),
    ABGR8(75, 175, 0, 255),
    ABGR8(100, 150, 0, 255),
    ABGR8(0, 192, 55, 255),

    ABGR8(0, 255, 125, 255),
    ABGR8(0, 150, 0, 255),
    ABGR8(0, 96, 0, 255),
    ABGR8(0, 64, 0, 255),

    // Cyans
    ABGR8(192, 255, 224, 255),
    ABGR8(148, 255, 228, 255),
    ABGR8(0, 255, 192, 255),
    ABGR8(0, 255, 255, 255), // Default P2, glow

    ABGR8(125, 255, 175, 255),
    ABGR8(67, 161, 138, 255),
    ABGR8(49, 109, 95, 255),
    ABGR8(38, 84, 73, 255),
    
    ABGR8(0, 150, 100, 255),
    ABGR8(0, 125, 125, 255),
    ABGR8(0, 96, 96, 255),
    ABGR8(0, 64, 64, 255),

    // Blues
    ABGR8(160, 255, 255, 255),
    ABGR8(0, 200, 255, 255),
    ABGR8(0, 125, 255, 255),
    ABGR8(0, 0, 255, 255),

    ABGR8(0, 75, 175, 255),
    ABGR8(0, 0, 150, 255),
    ABGR8(1, 7, 112, 255),
    ABGR8(0, 10, 76, 255),

    ABGR8(0, 100, 150, 255),
    ABGR8(0, 73, 109, 255),
    ABGR8(0, 50, 76, 255),
    ABGR8(0, 38, 56, 255),

    ABGR8(118, 189, 255, 255),
    ABGR8(80, 128, 173, 255),
    ABGR8(51, 83, 117, 255),
    ABGR8(35, 60, 86, 255),

    // Purples
    ABGR8(190, 181, 255, 255),
    ABGR8(125, 125, 255, 255),
    ABGR8(125, 0, 255, 255),
    ABGR8(100, 0, 150, 255),

    ABGR8(182, 128, 255, 255),
    ABGR8(75, 0, 175, 255),
    ABGR8(61, 6, 140, 255),
    ABGR8(55, 8, 96, 255),

    ABGR8(77, 77, 143, 255),
    ABGR8(111, 73, 164, 255),
    ABGR8(84, 54, 127, 255),
    ABGR8(66, 42, 99, 255),

    // Magentas
    ABGR8(252, 181, 255, 255),
    ABGR8(255, 0, 125, 255),
    ABGR8(150, 0, 100, 255),
    ABGR8(102, 3, 62, 255),

    ABGR8(255, 0, 255, 255),
    ABGR8(185, 0, 255, 255),
    ABGR8(125, 0, 125, 255),
    ABGR8(71, 1, 52, 255),

    ABGR8(250, 127, 255, 255),
    ABGR8(175, 87, 175, 255),
    ABGR8(130, 67, 130, 255),
    ABGR8(94, 49, 94, 255),

    // Gray scale
    ABGR8(255, 255, 255, 255),
    ABGR8(224, 224, 224, 255),
    ABGR8(175, 175, 175, 255),
    ABGR8(128, 128, 128, 255),
    ABGR8(90, 90, 90, 255),
    ABGR8(64, 64, 64, 255),
    ABGR8(0, 0, 0, 255)
};

extern const int gdToGd3dsColorTable[] = {
    38,
    39,
    44,
    51,
    62,
    63,
    78,
    92,
    89,
    3,
    14,
    26,
    100,
    93,
    29,
    106,
    61,
    102,
    104,
    1,
    43,
    57,
    64,
    81,
    94,
    8,
    16,
    27,
    41,
    15,
    20,
    30,
    42,
    56,
    68,
    79,
    90,
    4,
    45,
    65,
    52,
    77,
    25,
    96,
    50,
    31,
    32,
    80,
    2,
    84,
    67,
    0,
    76,
    5,
    6,
    7,
    9,
    10,
    11,
    17,
    18,
    19,
    12,
    13,
    21,
    22,
    23,
    33,
    34,
    35,
    24,
    28,
    36,
    37,
    48,
    49,
    53,
    54,
    55,
    46,
    47,
    58,
    59,
    60,
    66,
    69,
    70,
    71,
    73,
    74,
    75,
    101,
    82,
    83,
    105,
    85,
    86,
    87,
    88,
    97,
    98,
    95,
    103,
    91,
    99,
    40,
    72
};

extern const size_t numColors = ARRAY_LEN(colors);

/*
Categories:

Red,
Orange,
Yellow,
Green,
Cyan,
Blue,
Purple,
Magenta,
Gray
*/
static int categoryColors[] = {
    3,
    15,
    26,
    39,
    51,
    63,
    78,
    89,
    103
};

//amount of colors in each category
static int categoryCounts[] = {
    12,
    12,
    12,
    12,
    12,
    16,
    12,
    12,
    7
};

static int selectedCategory;

//the first color index in the selected category
static int startingColorIndex;

//used to increment category color buttons during setup
static int categoryCounter = 0;

//used to increment color buttons during setup
static int colorCounter = 0;

//color 1, 2, or glow
static int colorPage = 0;

static void disableGlowSetting(UIElement *e) {
    e->enabled = false;

    showGlow = (playerGlowEnabled || ((p1Color.r | p1Color.g | p1Color.b) == 0));
}

static void enableGlowSetting(UIElement *e) {
    e->enabled = true;
}

static void setColorIndex(UIElement *e) {
    UIColor *color = (UIColor *) e;

    int colorIndex = startingColorIndex + colorCounter;

    uiColorButtonSetIndex(color, colorPage, colorIndex);

    color->isSelected = *currentColors[colorPage] == colorIndex;

    e->enabled = colorCounter < categoryCounts[selectedCategory];

    colorCounter++;
}

static void setCategoryIndex(UIElement *e) {
    UIColor *color = (UIColor *) e;
    //rather than using the color index as col1/col2/glow, it's used as a category index for the button
    uiColorButtonSetIndex(color, categoryCounter, categoryColors[categoryCounter]);
    
    color->isSelected = selectedCategory == categoryCounter;
    
    categoryCounter++;
}

static void resetIndices(UIScreen *s){
    categoryCounter = 0;
    colorCounter = 0;

    //find first color index
    startingColorIndex = 0;
    for(int i = 0; i < selectedCategory; i++){
        startingColorIndex += categoryCounts[i];
    }

    uiRunFuncOnTag(s, "category", setCategoryIndex);
    uiRunFuncOnTag(s, "color", setColorIndex);
}

static void actionCategorySelected(UIElement *e, const UIPropertyList *args){
    UIColor *color = (UIColor *) e;

    selectedCategory = color->index;

    resetIndices(e->screen);
}

static void actionColorSelected(UIElement *e, const UIPropertyList *args) {
    UIColor *color = (UIColor *) e;

    *currentColors[colorPage] = color->colorIndex;
    updatePlayerColors();

    showGlow = (playerGlowEnabled || ((p1Color.r | p1Color.g | p1Color.b) == 0));

    resetIndices(e->screen);
}

static void disableAllColorButtons(UIElement *e) {
    uiWindowButtonSetStyle((UIWindowButton *) e, 4);
}

static void resetSelectedCategory(){
    //get selected category from selected color index
    int currentIndex = *currentColors[colorPage];

    //9 is the number of categories
    for(int i = 0; i < 9; i++){
        if(currentIndex < categoryCounts[i]){
            selectedCategory = i;
            break;
        }
        currentIndex -= categoryCounts[i];
    }
}

static void setP1Page(UIElement *e, const UIPropertyList *args) {    
    uiRunFuncOnTag(e->screen, "color_buttons", disableAllColorButtons);
    uiWindowButtonSetStyle((UIWindowButton *) e, 5);
    colorPage = 0;
    uiRunFuncOnTag(e->screen, "glow_option", disableGlowSetting);
    resetSelectedCategory();
    resetIndices(e->screen);
}

static void setP2Page(UIElement *e, const UIPropertyList *args) {
    uiRunFuncOnTag(e->screen, "color_buttons", disableAllColorButtons);
    uiWindowButtonSetStyle((UIWindowButton *) e, 5);
    colorPage = 1;
    uiRunFuncOnTag(e->screen, "glow_option", disableGlowSetting);
    resetSelectedCategory();
    resetIndices(e->screen);
}

static void setGlowPage(UIElement *e, const UIPropertyList *args) {
    uiRunFuncOnTag(e->screen, "color_buttons", disableAllColorButtons);
    uiWindowButtonSetStyle((UIWindowButton *) e, 5);
    colorPage = 2;
    uiRunFuncOnTag(e->screen, "glow_option", enableGlowSetting);
    resetSelectedCategory();
    resetIndices(e->screen);
}

void playerGlowSettings(UIElement* e, const UIPropertyList *args) {
    UICheckBox *checkbox = (UICheckBox *) e;
    playerGlowEnabled = checkbox->checked;
    showGlow = (playerGlowEnabled || ((p1Color.r | p1Color.g | p1Color.b) == 0));
}

static void paletteKitInit(UIScreen *s) {
    colorPage = 0;

    resetSelectedCategory();
    resetIndices(s);

    uiSetCheckboxChecked((UICheckBox *) uiGetElementByTag(s, "check_glow"), playerGlowEnabled);
    uiRunFuncOnTag(s, "glow_option", disableGlowSetting);
}

static const UIActionDef paletteKitActions[] = {
    {"color_selected", actionColorSelected},
    {"action_p1", setP1Page},
    {"action_p2", setP2Page},
    {"action_glow", setGlowPage},
    {"glow", playerGlowSettings},
    {"category_selected", actionCategorySelected}
};

extern const UIScreenDefPair paletteKitDef = {
    .name = "palette_kit",
    .top = {
        0
    },
    .btm = {
        .path = "romfs:/menus/palette_kit.txt",
        .init = paletteKitInit,
        .actionList = {
            .actions = paletteKitActions,
            .actionCount = ARRAY_LEN(paletteKitActions)
        }
    }
};