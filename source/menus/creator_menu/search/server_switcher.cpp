#include "menus/creator_menu/search/server_switcher.hpp"
#include <3ds.h>
#include <citro2d.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "mp3_player.hpp"

#include "utils/server_utils.hpp"

#include "menus/core/ui_stack.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_checkbox.hpp"
#include "menus/components/ui_window_button.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_window_button.hpp"
#include "state.hpp"
#include "menus/creator_menu/search_menu.hpp"

namespace gd3ds
{

static void darkenText(UIElement* e){
    UILabel *l = (UILabel *)e;

    if(uiPropInt(&e->parent->customProperties, "server", 0) != gdps) snprintf(l->text, sizeof(l->text), "<127, 127, 127>%s</>", uiPropString(&e->customProperties, "basetext", "Fuck you"));
    else snprintf(l->text, sizeof(l->text), "<255, 255, 255>%s</>", uiPropString(&e->customProperties, "basetext", "Fuck you"));
}

static void darkenImage(UIElement* e){
    UIImage *i = (UIImage *)e;

    if(uiPropInt(&e->parent->customProperties, "server", 0) != gdps) uiImageSetTint(i, C2D_Color32(127, 127, 127, 255));
    else uiImageSetTint(i, C2D_Color32(255, 255, 255, 255));
}

static void darkenButton(UIElement* e){
    UIWindowButton *b = (UIWindowButton *)e;

    if(uiPropInt(&e->customProperties, "server", 0) != gdps) uiWindowButtonSetTint(b, C2D_Color32(127, 127, 127, 255));
    else uiWindowButtonSetTint(b, C2D_Color32(255, 255, 255, 255));
}

static void updateServerButtons(UIScreen *s){
    uiRunFuncOnTag(s, "server", darkenButton);
    uiRunFuncOnTag(s, "serverimage", darkenImage);
    uiRunFuncOnTag(s, "servertext", darkenText);
}

static void actionSwitchServer(UIElement* e, const UIPropertyList *args) {
    int target = uiPropInt(&e->customProperties, "server", 0);
    bool gdpsBefore = gdps;
    gdps = (target == 1);

    if(gdps != gdpsBefore){
        stopMP3();
        strcpy(menuLoopPath, gdps ? "romfs:/songs/menuLoopGDPS.mp3" : "romfs:/songs/menuLoop.mp3");
        playingMenuLoop = false;
        playMenuSong();

        if (gdps) {
            filters.difficultyFilters = filters.isDemon ? 0 : filters.difficultyFilters;
        }

        if (uiStackCheckLoadedRoot(&searchMenuDef)) {
            UIScreenPair *screenPair = uiStackGetLoadedScreen(&searchMenuDef);
            UIScreen *screen = &screenPair->screens[SCREEN_BTM];
            if(gdps){
                disableDemons(screen);
            } else if(!gdps && filters.isDemon){
                enableDemons(screen);
            }

            updateDifficultyTints(screen);
        }

        filters.super = filters.super && gdps;
        filters.mainSong = 0;
        filters.songFilter = false;
    }

    updateServerButtons(e->screen);

    loadGdpsInfo();
}

static UIActionDef serverSwitcherActions[] = {
    { "change_server", actionSwitchServer}
};

void serverSwitcherInit(UIScreen *s) {
    updateServerButtons(s);
}

extern const UIScreenDefPair serverSwitcherDef = {
    .name = "server_switcher",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/server_switcher_pop_up.txt",
        .init = serverSwitcherInit,
        .actionList = {
            .actions = serverSwitcherActions,
            .actionCount = ARRAY_LEN(serverSwitcherActions)
        }
    }
};

}