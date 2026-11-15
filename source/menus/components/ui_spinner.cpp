#include "ui_spinner.hpp"
#include <citro3d.h>
#include "graphics.hpp"

#include "menus/core/ui_props.hpp"
#include "menus/core/ui_screen.hpp"
#include "ui_image.hpp"
#include "math_helpers.hpp"

#include "game/game_manager.hpp"

namespace gd3ds
{

static void uiSpinnerDraw(UIElement* e, UITransform *transform) {
    UIImage *image = (UIImage *) e;
    UISpinner *spinner = (UISpinner *) image;
    
    C2D_SpriteRotate(&image->image.sprite, C3D_AngleFromDegrees(spinner->rotationSpeed * GameManager::get()->deltaTime)); // Cloud pls save us all and do screen rewrite //Why would that help this
    
    changeBlending(spinner->blending);
    uiImageDraw(e, transform);
    changeBlending(false);
}

void uiSpinnerDestroy(UIElement *e) {
    (void)e;
}


UISpinner *uiCreateSpinner(UIScreen *s) {
    auto e = std::make_unique<UISpinner>();

    UISpinner *result = e.get();
    UIImage *image = (UIImage *) result;

    image->base.type = UIElementType::UiImage;
    image->base.enabled = true;

    uiElementApplyDefaultProperties(&image->base, s);

    uiImageClearTint(image);
    uiImageSetImage(image, 53, 1);

    result->blending = true;

    image->base.draw = uiSpinnerDraw;
    image->base.destroy = uiSpinnerDestroy;

    return e.release();
}

UIElement *uiCreateSpinnerFromProps(UIScreen *s, const UIPropertyList *props) {
    UISpinner *spinner = uiCreateSpinner(s);
    
    if (!spinner) return NULL;

    UIImage *image = (UIImage *) spinner;

    uiElementApplyProperties(&image->base, s, props);

    uiImageSetImage(image, 
        uiPropInt(props, "id", 53),
        uiPropInt(props, "sheet", 1)
    );
    
    uiImageSetTint(image, uiPropColor(props, "color", ABGR8(255, 255, 255, 200)));

    spinner->rotationSpeed = uiPropFloat(props, "speed", 360);
    spinner->blending = uiPropBool(props, "blending", true);

    return &image->base;
}

}