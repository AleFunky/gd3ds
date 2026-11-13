#include <3ds.h>
#include <citro2d.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "utils/server_utils.hpp"

namespace gd3ds
{

static void updateLengthTint(UIElement *e){
    UILabel *l = (UILabel *)e;
    int opacity = (filters.lengthFilters & (uiPropInt(&e->customProperties, "lengthval", 0))) > 0 ? 255 : 127;

    int lengthBit = uiPropInt(&e->customProperties, "lengthval", 0);
    int length = __builtin_ctz(lengthBit);

    const char *lengthStr = "Unkn.";
    if (IN_BOUNDS((size_t)length, levelLengths)) {
        lengthStr = (const char *) levelLengths[length];
    }

    snprintf(l->text, sizeof(l->text), "<%d,%d,%d>%s</>", opacity, opacity, opacity, lengthStr);
}

static void updateLengthTints(UIScreen *s){
    uiRunFuncOnTag(s, "lengthbtn", updateLengthTint);
}

static void actionSetLength(UIElement* e, const UIPropertyList *props) {
    filters.lengthFilters ^= uiPropInt(&e->customProperties, "lengthval", 0);
    updateLengthTints(e->screen);
}

static UIActionDef lengthFilterActions[] = {
    { "length", actionSetLength },
};

void lengthFilterInit(UIScreen *s) {
    updateLengthTints(s);
}

extern const UIScreenDefPair lengthFilterDef = {
    .name = "length_filter",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/length_filter_pop_up.txt",
        .init = lengthFilterInit,
        .actionList = {
            .actions = lengthFilterActions,
            .actionCount = ARRAY_LEN(lengthFilterActions)
        }
    }
};

}