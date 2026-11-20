#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/core/ui_stack.hpp"
#include "text.hpp"

#include "menus/settings_hub/info_card.hpp"

#include "fonts/chatFont.hpp"

namespace gd3ds
{

void setInfoTitle(const char *text, UIScreen *s) {
    UILabel *title = (UILabel *) uiGetElementByTag(s, "title");
    uiLabelSetText(title, text);
}

void setInfoContent(const char *text, UIScreen *s) {
    UILabel *content = (UILabel *) uiGetElementByTag(s, "content");
    UIWindow *window = (UIWindow *) uiGetElementByTag(s, "window");
    UIWindowButton *button = (UIWindowButton *) uiGetElementByTag(s, "button");
    UILabel *title = (UILabel *) uiGetElementByTag(s, "title");
    float textMarginX = 15.f;
    float textScale = content->base.scaleX;
    float buttonOffset = 30.f;
    float titleOffset = 28.f;
    
    char *buf = wrapText(&chatFontFontCharset, textScale, text, window->base.w - 2 * textMarginX);
    float textHeight = getTextHeight(&chatFontFontCharset, &chatFontSheet, textScale, true, buf);

    float windowHeight = 90.f + textHeight;

    window->base.h = windowHeight;

    button->base.base.y = (windowHeight / 2) - buttonOffset;
    title->base.y = -(windowHeight / 2) + titleOffset;
    uiLabelSetText(content, buf);
}

void infoCardInit(UIScreen *s) {
    InfoCardData *infoCardData = (InfoCardData *)s->pair->data;
    if(!infoCardData) return;
    setInfoContent(infoCardData->text, s);
    if(infoCardData->customTitle) setInfoTitle(infoCardData->title, s);
}

void infoCardFreeData(void *data){
    InfoCardData *infoCardData = (InfoCardData *)data;
    if(!infoCardData) return;
    if(infoCardData->copied) free((void *)infoCardData->text);
    if(infoCardData->customTitle) free((void *)infoCardData->title);
    free(infoCardData);
}

extern const UIScreenDefPair infoCardDef = {
    .name = "info_card",
    .btm = {
        .path = "romfs:/menus/settings_hub/info_card.txt",
        .init = infoCardInit
    },
    .freeData = infoCardFreeData
};

}