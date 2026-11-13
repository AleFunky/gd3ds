#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

#include "level/main_levels.hpp"
#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_darken.hpp"
#include "menus/components/ui_rectangle.hpp"
#include "menus/components/ui_label.hpp"

#include "menus/statistics.hpp"

#include "save/saving.hpp"
#include "menus/settings_hub/songs.hpp"

static UIList *list;

static void addEntry(const char *title, const char *artist, float listWidth, UIScreen *s, int i) {

    UIElement *card = (UIElement *) uiCreateRectangle(s);

    if (card) {
        uiRectangleSetColor((UIRectangle *) card, (i & 1 ? C2D_Color32(194,114,62,255) :  C2D_Color32(161,88,48,255)));
        uiElementSetSize(card, 0, 46);

        // Song name
        UILabel *song = uiCreateLabel(s);
        if (song) {
            song->base.w = list->base.w - 12;
            uiLabelSetText(song, title);
            uiElementSetPosition((UIElement *) song, -listWidth + 6, - 5);
            uiElementSetScale((UIElement *) song, 0.54f);
            
            // song->font = 2;

            uiElementAddChild(card, (UIElement *) song);
        }

        // Song name
        UILabel *creator = uiCreateLabel(s);
        if (creator) {
            uiLabelSetText(creator, artist);
            uiElementSetPosition((UIElement *) creator, -listWidth + 6, + 7);
            uiElementSetScale((UIElement *) creator, 0.54f);
            
            creator->font = 2;

            uiElementAddChild(card, (UIElement *) creator);
        }

        uiListAdd(list, card);
    }
}

void songsInit(UIScreen *s) {
    list = (UIList *) uiGetElementByTag(s, "list");
    if (list) {
        float listWidth = list->base.w * 0.5f;
        size_t i = 0;
        for (; i < currentMainLevelPack->count; i++) {
            const char *title = currentMainLevelPack->levels[i].songData.title;
            const char *artist = currentMainLevelPack->levels[i].songData.artist;

            addEntry(title, artist, listWidth, s, i);
        }
        
        addEntry("Practice: Stay Inside Me", "OcularNebula", listWidth, s, i);
    }
}

extern const UIScreenDefPair songsDef = {
    .name = "songs",
    .btm = {
        .path = "romfs:/menus/settings_hub/soundtrack.txt",
        .init = songsInit,
    }
};