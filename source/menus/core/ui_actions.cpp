#include "sounds.hpp"
#include "ui_element.hpp"
#include "wav_player.hpp"
#include "ui_actions.hpp"

namespace gd3ds
{

const UIIntEnumEntry animEnum[] = {
    { "none", static_cast<int>(UIAnimation::AnimNone) },
    { "slide_down", static_cast<int>(UIAnimation::AnimSlideDown) },
    { "slide_right", static_cast<int>(UIAnimation::AnimSlideRight) },
    { "zoom", static_cast<int>(UIAnimation::AnimZoom) },
    { "zoom_subtle", static_cast<int>(UIAnimation::AnimZoomSubtle) },
    { "bounce_down", static_cast<int>(UIAnimation::AnimBounceDown) },
    { "bounce_down_slow", static_cast<int>(UIAnimation::AnimBounceDownSlow) },
};

const UIIntEnumEntry screenEnum[] = {
    { "top", static_cast<int>(Screens::ScreenTop) },
    { "btm", static_cast<int>(Screens::ScreenBtm) }
};

const UIIntEnumEntry pushEnum[] = {
    { "next", static_cast<int>(UIStackPushType::PushNext) },
    { "after_close", static_cast<int>(UIStackPushType::PushAfterClose) },
    { "root", static_cast<int>(UIStackPushType::PushRoot) }
};

static void actionOpenMenu(UIElement *e, const UIPropertyList *args){
    UIAnimation topAnim = (UIAnimation)uiPropIntEnum(args, "top_anim", animEnum, ARRAY_LEN(animEnum), static_cast<int>(UIAnimation::AnimNone));
    UIAnimation btmAnim = (UIAnimation)uiPropIntEnum(args, "btm_anim", animEnum, ARRAY_LEN(animEnum), static_cast<int>(UIAnimation::AnimNone));
    uiStackPushName(uiPropString(args, "screen", "soggy"), topAnim, btmAnim, (UIStackPushType)uiPropIntEnum(args, "type", pushEnum, ARRAY_LEN(pushEnum), static_cast<int>(UIStackPushType::PushNext)));
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

}