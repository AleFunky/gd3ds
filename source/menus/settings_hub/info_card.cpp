#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/core/ui_stack.hpp"

#include "menus/settings_hub/info_card.hpp"

namespace gd3ds
{

void setInfoTitle(const char *text, UIScreen *s) {
    UILabel *title = (UILabel *) uiGetElementByTag(s, "title");
    uiLabelSetText(title, text);
}

void setInfoContent(const char *text, UIScreen *s) {
    UILabel *content = (UILabel *) uiGetElementByTag(s, "content");
    uiLabelSetText(content, text);
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