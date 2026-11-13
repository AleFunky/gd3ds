#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_button.hpp"

#include "menus/main_menu.hpp"

namespace gd3ds
{

static size_t currentPage = 0;

static UIWindowButton *nextButton;

const char *howToPlayPages[] = {
    "page1",
    "page2",
    "page3",
    "page4",
    "page5",
    "page6",
    "page7",
    "page8",
};


void switchPage2(size_t page, UIScreen *s) {
    for (size_t i = 0; i < ARRAY_LEN(howToPlayPages); i++) {
        if (i == page) {
            uiRunFuncOnTag(s, howToPlayPages[page], uiEnableElement);
            uiRunFuncOnTag(&s->pair->screens[SCREEN_TOP], howToPlayPages[page], uiEnableElement);

        } else {
            uiRunFuncOnTag(s, howToPlayPages[i], uiDisableElement);
            uiRunFuncOnTag(&s->pair->screens[SCREEN_TOP], howToPlayPages[i], uiDisableElement);
        }
    }
}

void actionGoNext(UIElement *e, const UIPropertyList *args) {
    currentPage++;
    if (currentPage >= ARRAY_LEN(howToPlayPages)) {
        uiStackPop();
    }

    switchPage2(currentPage, e->screen);
}


static UIActionDef howToPlayActions[] = {
    { "next", actionGoNext}
};

void howToPlayInit(UIScreen *s) {
    nextButton = (UIWindowButton*) uiGetElementByTag(s, "nextbutton");
    currentPage = 0;

    switchPage2(currentPage, s);
}

static void howToPlayUpdate(UIScreen *s, UIInput *touch) {
    if (currentPage == 7) uiButtonSetText((UIButton *)nextButton, "Exit");
}

extern const UIScreenDefPair howToPlayDef = {
    .name = "how_to_play",
    .top = {
        .path = "romfs:/menus/settings_hub/how_to_play_top.txt",
    },
    .btm = {
        .path = "romfs:/menus/settings_hub/how_to_play.txt",
        .init = howToPlayInit,
        .update = howToPlayUpdate,
        .actionList = {
            .actions = howToPlayActions,
            .actionCount = ARRAY_LEN(howToPlayActions)
        }
    }
};

}