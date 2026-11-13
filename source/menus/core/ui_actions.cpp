#include "sounds.hpp"
#include "ui_element.hpp"
#include "wav_player.hpp"
#include "ui_actions.hpp"

const UIIntEnumEntry animEnum[] = {
    { "none", ANIM_NONE },
    { "slide_down", ANIM_SLIDE_DOWN },
    { "slide_right", ANIM_SLIDE_RIGHT },
    { "zoom", ANIM_ZOOM },
    { "zoom_subtle", ANIM_ZOOM_SUBTLE },
    { "bounce_down", ANIM_BOUNCE_DOWN },
    { "bounce_down_slow", ANIM_BOUNCE_DOWN_SLOW },
};

const UIIntEnumEntry screenEnum[] = {
    { "top", SCREEN_TOP },
    { "btm", SCREEN_BTM }
};

const UIIntEnumEntry pushEnum[] = {
    { "next", PUSH_NEXT },
    { "after_close", PUSH_AFTER_CLOSE },
    { "root", PUSH_ROOT }
};

static void actionOpenMenu(UIElement *e, const UIPropertyList *args){
    UIAnimation topAnim = (UIAnimation)uiPropIntEnum(args, "top_anim", animEnum, ARRAY_LEN(animEnum), ANIM_NONE);
    UIAnimation btmAnim = (UIAnimation)uiPropIntEnum(args, "btm_anim", animEnum, ARRAY_LEN(animEnum), ANIM_NONE);
    uiStackPushName(uiPropString(args, "screen", "soggy"), topAnim, btmAnim, (UIStackPushType)uiPropIntEnum(args, "type", pushEnum, ARRAY_LEN(pushEnum), PUSH_NEXT));
}

static void actionCloseMenu(UIElement *e, const UIPropertyList *args){
    uiStackPop();
}

static void actionCloseContext(UIElement *e, const UIPropertyList *args){
    uiStackPopContext();
}

static void actionDisableTag(UIElement *e, const UIPropertyList *args){
    int screen = uiPropIntEnum(args, "screen", screenEnum, ARRAY_LEN(screenEnum), -1);
    if(screen >= 0 && screen < 2){
        uiRunFuncOnTag(&e->screen->pair->screens[screen], uiPropString(args, "tag", ";,[]{}Nope"), uiDisableElement);
        return;
    }
    uiRunFuncOnTag(e->screen, uiPropString(args, "tag", "unused"), uiDisableElement);
}

static void actionEnableTag(UIElement *e, const UIPropertyList *args){
    int screen = uiPropIntEnum(args, "screen", screenEnum, ARRAY_LEN(screenEnum), -1);
    if(screen >= 0 && screen < 2){
        uiRunFuncOnTag(&e->screen->pair->screens[screen], uiPropString(args, "tag", ";,[]{}Nope"), uiEnableElement);
        return;
    }
    uiRunFuncOnTag(e->screen, uiPropString(args, "tag", "unused"), uiEnableElement);
}

static void actionPlaySound(UIElement *e, const UIPropertyList *args){
    playSfx(getSound(uiPropString(args, "sound", "boop")), uiPropInt(args, "channel", 1));
}

extern const UIActionDef baseActions[] = {
    { "open_menu", actionOpenMenu },
    { "close_menu", actionCloseMenu },
    { "close_context", actionCloseContext },
    { "disable_tag", actionDisableTag },
    { "enable_tag", actionEnableTag },
    { "play_sound", actionPlaySound }
};
