#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.h"
#include "math_helpers.h"
#include "menus/components/ui_image.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_progress_bar.h"
#include "main.h"
#include "triggers.h"
#include "graphics.h"

static UIProgressBar *progressbar;
static UILabel *splashtext;

char *splash_texts[] = {
    "0% official",
    "2.0 Coming NOW!",
    "2.0 compatible!",
    "2.1 when",
    "2.3 when?",
    "99% accuracy!",
    "500 memory leaks",
    "1969 flipped is 1969",
    "#advyout",
    "%sReplacedAdded;=",
    "//AleAle",
    "/\\/\\/\\",
    "@ParticleGPT",
    "\"ok but 2.2 whe-\" get out.",
    "AAAAAAAAAAAAAAAAAAA",
    "add ground",
    "Adding edge cases...",
    "Adding memory leaks...",
    "advexed never clear",
    "Ale hears you...",
    "AleFunky x RobTop phonk",
    "All hail linear filtering",
    "Also try Famidash!",
    "Also try Geometry Dash Advance!",
    "Also try Super Famidash!",
    "Also try The Impossible Game",
    "An exception has occurred",
    "and Knuckles!",
    "And that's how I met your mother",
    "anything but square adventure",
    "As seen on the Famicom!",
    "As seen on the GameBoy Advance!",
    "As seen on your computer!",
    "Base After Base? Who's that?",
    "BBQ chicken alert",
    "Better than Fortnite, fight me Epic",
    "Better than GDA!",
    "Bwomp",
    "By: Dimrain47",
    "Can you do um maybe like um... stuff",
    "ChatGPT can you add 2.2 and make it run at 1000fps?",
    "Check out other TFDSoft projects!",
    "Coins are not convertible to real life currency",
    "Como descargar Famidash para Android en 2024",
    "CONCRETE",
    "Congrats!",
    "Contacting ChatGPT...",
    "Dash: Jumping Evolved",
    "Did you know: Geometry dash is based on a true story-",
    "Did you remember to flush the toilet",
    "Do you wanna have a bad time?",
    "Does this look possible to you?",
    "Doing nothing...",
    "Don't ask alefunky for 2.1 at 3 AM!!",
    "don't break your screen please",
    "Don't hug spikes, please.",
    "Don't lose your stylus!",
    "Dont make dumb issues. Please.",
    "Due to the large count of characters, and the huge amount of content this specific splash has, you really can't see this entire message on your 3DS screen due to the pixelation it's getting. Isn't that weird, right?",
    "Each sold seperately",
    "Eat grass",
    "Eating 3DS ram...",
    "Editor isn’t coming!",
    "Enabling camera...",
    "Enabling microphone...",
    "Enveloping...",
    "Erasing save data...",
    "Erm. Do i have to save the file?",
    "Error 404 - Game not found",
    "Famidash Retray 100% ALL COINS",
    "FIVE FIVE. FIVE FIVE.",
    "Fixing bugs...",
    "Focus C.",
    "Formatting SD card...",
    "Free and Open Source!",
    "free(people_array[get_SSN(\"KandoWontu\")])",
    "Get ready for a lot of fun and excitement!",
    "Gg frickin ez",
    "GJ means GeometryJump",
    "GOD DAMNIT KRIS WHERE THE HELL ARE WE",
    "Hacking your 3DS...",
    "Happy 2004!",
    "Have you encountered the nightmare men?",
    "Hello, Mario.",
    "hey it's me it's verity",
    "Hope you don't mind 45 seconds of input lag",
    "horse",
    "How about you Geometry Dash to your shower",
    "How mfs look at you when your\nfirst extreme is Bloodbath\nand not \"MLG Mountain Dew Lasagna XVII\"",
    "I AM NOT A MORON!!!!",
    "I hear voices",
    "I love GD cologne",
    "I summon the 98% curse!",
    "I want the fart gamemode",
    "I'm on my way, to happiness and misery to be. uh huh uh huh uh huh uh huh",
    "If you're expecting 2.2, delete the game immediately.",
    "In my sog era, sogging it up pretty hard...",
    "Is it fixed",
    "IS THAT A GEOMETRY DASH REFERENCE???",
    "It was hard before... Now it’s even harder!",
    "It's showtime",
    "Its mostly optimisation",
    "Jaundice square project",
    "Jesse, the gpu and cpu ARE parallel",
    "joe mother",
    "Just Play The Real GD Bro",
    "Lethal lava land",
    "Loading 1's and 0's...",
    "Loading hopes and dreams...",
    "Loading malware...",
    "Loading nothing in particular...",
    "Loading potatodash...",
    "Loading random code...",
    "Loading ReTraY Dash...",
    "Loading terrible download speeds...",
    "Loading the progress bar...",
    "Look ma! no rotate triggers!",
    "Mango Mango Mango Mango",
    "meow",
    "Minecraft time!",
    "Mining Bitcoin...",
    "More games to never touch on your 3ds!",
    "Move trigger? More like slopes the second",
    "Music by... too many to list here",
    "MY GOD THE SPEED! THE SPEED!",
    "No consistent naming conventions!",
    "Node Graph out of Date. Rebuilding...",
    "Not vibecoded!", // Hopefully reduces people that think this the game is vibecoded lol (okay it turns out this is message number 67, how interesting)
    "Now in potato hardware!",
    "Now on Nintendo 3DS!",
    "Now with accurate physics!",
    "Now with nausea inducing 3D!",
    "Now with random move trigger deaths!",
    "Now With(out) a Gyroscope!",
    "Now without parental controls!",
    "o:",
    "Of course you got on.",
    "Oh hey, you again!",
    "ok",
    "OMG OMG OMG OMFG",
    "Pathfinder training",
    "Platformer? I barely know her!",
    "Play dashmetry",
    "Please deposit five coins...",
    "Pray to the slope gods",
    "Presented by AleTop",
    "pretend i said something funny so i can get top comment",
    "Probably runs at like... 5FPS",
    "Progress Alert",
    "Put ya 3DS away Waltuh",
    "Removing Herobrine...",
    "Renaming game to \"America dash\"...",
    "Setup is starting GD3DS...",
    "she G on my D till i A",
    "Since it's 2 screens, is it 2 completions if you beat a level?",
    "Six Seven!",
    "Slope slope slope",
    "So when are you beating Grief",
    "So... how's your mom been?",
    "Someone help, I'm stuck in the code!",
    "stereo madness day 832 97x2",
    "Such wow",
    "The dart walked so the wave could run",
    "The rage game but now you can snap some hinges!",
    "THE TRAIN!!!!!!!!",
    "this demake has (not) been sponsored by NordVPN!",
    "This game is about jumping",
    "This is like that time I was in Geometry Dash",
    "This sentence is false",
    "thoust worms shall dash geometry",
    "TIP: Jump to Every End",
    "TIP: Press F1 for help.",
    "Trans rights are human rights",
    "Uh. When super famidash",
    "uhh its loading again",
    "Untangling the spaghetti...",
    "Update 2.0 is revolution!",
    "VERY VERY INTERESTING",
    "void envelop_objects();",
    "Waiter waiter, more revamps please",
    "Wake up, grief is verified",
    "We are Geometry Dash!!!",
    "We can assure you Advy hasn't touched this game! Wait...",
    "we do eht",
    "We went from Rob Bottom to Rob Top!",
    "Welcome to 3DS modding",
    "Welcome to robert game",
    "Well, well, well. Look who's back.",
    "What does this do",
    "What is this game?",
    "When the verification video is lost media",
    "who is cloud5474",
    "Who knew Geometry could Dash?",
    "Wiidash walked so GD3DS could run!",
    "Wishing you could install Geode, huh?",
    "With 2x less the processing power!",
    "Wouldn't it be funny if this was\nin the Oxygene1 font -Crafty Jumper",
    "Yeah Aeternus is probably Grandelite tier",
    "You cant beat Deadlocked? Git gud",
    "You will fluke it. Don't worry",
    "You wouldnt steal a geometry dash",
    "You're playing on a what!?"
};

void loading_screen_init() {
    ui_load_screen_old(&default_screen, NULL, 0, "romfs:/menus/loading_screen.txt");
    ui_load_screen_old(&default_screen_top, NULL, 0, "romfs:/menus/loading_screen_top.txt");

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

void loading_screen_update(float progress, int fade) {    
    progressbar->value = progress;
    UIInput touch;
    touchPosition touchPos;
    hidTouchRead(&touchPos);
    touch.touchPosition = touchPos;
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
    
        C2D_DrawRectSolid(0.f, 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, fade));
    }

    // Bottom Screen
    C2D_TargetClear(bot, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(bot);

    draw_background(20, SCREEN_HEIGHT-30);
    C2D_ViewScale(SCALE, SCALE);

    C2D_ViewScale(1/SCALE, 1/SCALE);
    ui_screen_draw(&default_screen);

    C2D_DrawRectSolid(0.f, 0.f, 0.f, SCREEN_WIDTH, SCREEN_HEIGHT, C2D_Color32(0, 0, 0, fade));

    C2D_ViewReset();
    C3D_FrameEnd(0);
}
