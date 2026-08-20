#include <3ds.h>
#include <citro2d.h>
#include "menus/core/ui_element.h"
#include "menus/core/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_progress_bar.h"
#include "main.h"
#include "color_channels.h"
#include "graphics.h"

static UIProgressBar *progressbar;
static UILabel *splashtext;

char *splash_texts[] = {
    "Welcome to robert game",
    "Slope slope slope",
    "Now in potato hardware!",
    "Now with accurate physics!",
    "All hail linear filtering",
    "Famidash Retray 100% ALL COINS",
    "ok",
    "add ground",
    "@ParticleGPT",
    "Also try Geometry Dash Advance!",
    "I want the fart gamemode",
    "2.0 when",
    "Look ma! no move triggers!",
    "99% accuracy!",
    "Six Seven!",
    "AAAAAAAAAAAAAAAAAAA",
    "Progress Alert",
    "Jesse, the gpu and cpu ARE parallel",
    "More games to never touch on your 3ds!",
    "You wouldnt steal a geometry dash",
    "Free and Open Source!",
    "2.0 Coming never!",
    "Wiidash walked so GD3DS could run!",
    "Also try Famidash!",
    "Check out other TFDSoft projects!",
    "Dont make dumb issues. Please.",
    "Formatting SD card...",
    "Is it fixed",
    "Uh. When super famidash",
    "Pray to the slope gods",
    "Untangling the spaghetti...",
    "Erm. Do i have to save the file?",
    "Happy 2004!",
    "Como descargar Famidash para Android en 2024",
    "Lethal lava land",
    "we do eht",
    "MY GOD THE SPEED! THE SPEED!",
    "I love GD cologne",
    "You cant beat TOE 2? Git gud",
    "When the verification video is lost media",
    "Music by... too many to list here",
    "Update 1.9 is revolution!",
    "1.9 compatible!",
    "/\\/\\/\\",
    "The dart walked so the wave could run",
    "Get ready for a lot of fun and excitement!",
    "Pathfinder training",
    "Does this look possible to you?",
    "Bwomp",
    "Trans rights are human rights",
    "No consistent naming conventions!"
};

static UIAction actions[] = {

};

static UIAction actions_top[] = {

};

void loading_screen_init() {
    ui_load_screen(&default_screen, actions, sizeof(actions) / sizeof(actions[0]), "romfs:/menus/loading_screen.txt");
    ui_load_screen(&default_screen_top, actions_top, sizeof(actions_top) / sizeof(actions_top[0]), "romfs:/menus/loading_screen_top.txt");

    Color col;
    col.r = 0;
    col.g = 102;
    col.b = 255;

    int chan = get_col_channel_index(CHANNEL_BG);

    channels[chan].color = col;
    get_buffer(chan)->active = false;

    handle_col_channel(chan);

    UIImage *title = (UIImage *) ui_get_element_by_tag(&default_screen_top, "title");

    if (title && alt_title_screen) {
        ui_image_set_image(title, 3, 1);
    }
    
    progressbar = (UIProgressBar *) ui_get_element_by_tag(&default_screen_top, "loadprogress");
    ui_progress_bar_set_tint(progressbar, C2D_Color32(50, 190, 240, 255));
    progressbar->max_value = 100;

    splashtext = (UILabel *) ui_get_element_by_tag(&default_screen_top, "splashtext");
    
    int text_index = random_int(0, ARRAY_LEN(splash_texts) - 1);

    char *text = splash_texts[text_index];

    ui_label_set_text(splashtext, text);
}

void loading_screen_update(float progress) {    
    progressbar->value = progress;
    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
    touch.did_something = false;
    touch.interacted = false;

    ui_screen_update(&default_screen_top, &touch);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    
    // Top screen, drawn once per eye when 3D is on
    for (int eye = 0; begin_top_eye(eye); eye++) {
        begin_eye_layer(DEPTH_BACKGROUND);
        draw_background(-20, -30);
        end_eye_layer();

        begin_eye_layer(DEPTH_UI);
        ui_screen_draw(&default_screen_top);
        end_eye_layer();
    }

    // Bottom Screen
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(bot);

    draw_background(-20, SCREEN_HEIGHT-30);
    C2D_ViewScale(SCALE, SCALE);
    draw_fade();

    C2D_ViewScale(1/SCALE, 1/SCALE);
    ui_screen_draw(&default_screen);
    C2D_ViewReset();
    C3D_FrameEnd(0);
}
