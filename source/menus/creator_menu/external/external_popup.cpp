#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#include "level/main_levels.hpp"
#include "main.hpp"
#include "menus/core/ui_element.hpp"
#include "menus/settings_hub/info_card.hpp"
#include "mp3_player.hpp"
#include "state.hpp"
#include "save/saving.hpp"
#include "utils/string_helpers.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_progress_bar.hpp"
#include "menus/creator_menu/external/external_levels.hpp"
#include "menus/settings_hub/songs.hpp"
#include "menus/creator_menu/external/external_level_infobox.hpp"
#include "menus/creator_menu/external/external_popup.hpp"
#include "utils/utils.hpp"

#include "fonts/chatFont.hpp"

namespace gd3ds
{

static UIScreen *screen;
static UIScreen *screenTop;

static UILabel *levelName;
static UILabel *creatorName;
static UILabel *description;

static UILabel *downloadsLabel;
static UILabel *likesLabel;
static UILabel *starsLabel;
static UILabel *levelIDLabel;
static UILabel *songLabel;

static UIImage *difficultyFace;
static UIImage *likeImage;

static UIProgressBar *normalProgress;
static UILabel *normalProgressVal;
static UIProgressBar *practiceProgress;
static UILabel *practiceProgressVal;

static Thread thread;

static int starsNum = 0;

const int difficultyStars[MAX_STARS + 1] = {
    NA_FACE,
    AUTO_FACE,
    EASY_FACE,
    NORMAL_FACE,
    HARD_FACE,
    HARD_FACE,
    HARDER_FACE,
    HARDER_FACE,
    INSANE_FACE,
    INSANE_FACE,
    DEMON_FACE
};

static char *gmd = NULL;


static void openLevel(UIElement *e, const UIPropertyList *args) {
    state.customLevel = true;
    state.onlineLevel = false;

    stopMP3();
    playingMenuLoop = false;
    uiStackPushGameState(STATE_GAME);
}

static UIActionDef externalPopupActions[] = {
    { "play", openLevel },
};

static void setProgress() {
    normalProgress->value = currentLevelEntry->data.normalProgress;
    practiceProgress->value = currentLevelEntry->data.practiceProgress;

    char normal[32];
    char practice[32];
    snprintf(normal, sizeof(normal), "%d%%", currentLevelEntry->data.normalProgress);
    snprintf(practice, sizeof(practice), "%d%%", currentLevelEntry->data.practiceProgress);

    uiLabelSetText(normalProgressVal, normal);
    uiLabelSetText(practiceProgressVal, practice);
}

static void setNameCreator(char *gmd) {
    char *name = extractGMDKey((const char *) gmd, "k2", "s");
    if (name) {
        uiLabelSetText(levelName, name);
        snprintf(levelInfo.levelName, sizeof(levelInfo.levelName), "%s", name);
        free(name);
    } else {
        snprintf(levelInfo.levelName, sizeof(levelInfo.levelName), "%s", defaultName);
    }

    char tmp[512];
    char *creator = extractGMDKey((const char *) gmd, "k5", "s");
    if (creator) {
        snprintf(tmp, sizeof(tmp), "By %s", creator);
        uiLabelSetText(creatorName, tmp);
        snprintf(levelInfo.creatorName, sizeof(levelInfo.creatorName), "%s", creator);
        free(creator);
    } else {
        snprintf(levelInfo.creatorName, sizeof(levelInfo.creatorName), "%s", defaultName);
    }

    // Load data
    char file[516];
    snprintf(file, sizeof(file), "ext_%s_%s", levelInfo.levelName, levelInfo.creatorName);
    
    currentLevelEntry = getOrAddLevelToExternalFile(&externalFile, file);
    currentLevelList = LEVEL_LIST_EXTERNAL;
    
    setProgress();
}

static void setDescription(char *gmd) {
    char *desc = extractGMDKey((const char *)gmd, "k3", "s");
    if (!desc)
        return;

    bool doubleDecode = strncmp(gmd, "<?xml version", 13) != 0;

    fixBase64URL(desc);
    unsigned char *decoded = (unsigned char *)malloc(strlen(desc) + 1);
    int decodedLen = base64Decode(desc, decoded);

    if (decodedLen > 0) {
        decoded[decodedLen] = '\0';

        // If the gmd doesn't have <?xml version at the start, the description is encoded twice in base 64
        if (doubleDecode) {
            fixBase64URL((char *)decoded);

            unsigned char *decoded2 = (unsigned char *)malloc(decodedLen + 1);
            int decoded2Len = base64Decode((char *)decoded, decoded2);

            free(decoded);

            if (decoded2Len > 0) {
                decoded2[decoded2Len] = '\0';

                char *wrapped = wrapText(&chatFontFontCharset, description->base.scaleX, (char *)decoded2, MAX_DESCRIPTION_WIDTH);

                uiLabelSetText(description, wrapped);
            }

            free(decoded2);
        // Normal description, as it should be
        } else {
            char *wrapped = wrapText(&chatFontFontCharset, description->base.scaleX, (char *)decoded, MAX_DESCRIPTION_WIDTH);

            uiLabelSetText(description, wrapped);

            free(decoded);
        }
    } else {
        free(decoded);
    }

    free(desc);
}

static void setDownloadsLikes(char *gmd) {
    char *downloads = extractGMDKey((const char *) gmd, "k11", "i");
    if (downloads) {
        int downloadCount = atoi(downloads);
        char *truncatedDownloads = truncateNumber(downloadCount);
        free(downloads);

        if (!truncatedDownloads) return;

        uiLabelSetText(downloadsLabel, truncatedDownloads);    
        free(truncatedDownloads);
    }

    char *likes = extractGMDKey((const char *) gmd, "k22", "i");
    if (likes) {
        int likeCount = atoi(likes);
        char *truncatedLikes = truncateNumber(likeCount);
        free(likes);

        if (!truncatedLikes) return;

        uiLabelSetText(likesLabel, truncatedLikes);

        if (likeCount < 0) {
            uiImageSetImage(likeImage, DISLIKE_ICON, 0);
        }
        free(truncatedLikes);
    }
}

static void setStars(char *gmd) {
    char *stars = extractGMDKey((const char *) gmd, "k26", "i");
    if (stars) {
        uiLabelSetText(starsLabel, stars);
        starsNum = atoi(stars);
        
        // Clamp
        if (starsNum > MAX_STARS) {
            starsNum = 0;
        }

        if (currentLevelEntry->data.stars != starsNum) {
            currentLevelEntry->data.stars = starsNum;
        }

        free(stars);
    } else {
        starsNum = 0;
    }
}

static void setLevelID(char *gmd) {
    char tmp[512];

    char *levelID = extractGMDKey((const char *) gmd, "k1", "i");
    if (levelID) {
        snprintf(tmp, sizeof(tmp), "<#3F2215>ID: %s</>", levelID);
        free(levelID);

        uiLabelSetText(levelIDLabel, tmp);
    }
}

static void setSongID(char *gmd) {
    char tmp[512];

    int customSongID = -1;
    size_t songID = 0;

    char *gmdCustomSongID = extractGMDKey((const char *) gmd, "k45", "i");
    if (gmdCustomSongID) {
        customSongID = atoi(gmdCustomSongID); // Custom song id
        free(gmdCustomSongID);
    }
    char *gmdSongID = extractGMDKey((const char *) gmd, "k8", "i");
    if (gmdSongID) {
        songID = atoi(gmdSongID); // Official song id
        free(gmdSongID);
    }

    if (customSongID > 0) {
        if (checkSong(customSongID)) {
            snprintf(tmp, sizeof(tmp), "Using song: %d.mp3", customSongID);
        } else {
            snprintf(tmp, sizeof(tmp), "Using song: %d.mp3 (NOT FOUND)", customSongID);
        }
    } else {
        const char *songName = "Unknown";
        if (songID >= 0 && songID < currentMainLevelPack->count) {
            songName = currentMainLevelPack->levels[songID].songData.title;
        }
        snprintf(tmp, sizeof(tmp), "Using song: %s", songName);
    }

    uiLabelSetText(songLabel, tmp);
}

static void setDifficulty() {
    int face = difficultyStars[0];
    if (IN_BOUNDS((size_t)starsNum, difficultyStars)) {
        face = difficultyStars[starsNum];
    }
    uiImageSetImage(difficultyFace, face, 0);
}


static int loadGMD(GenericTask *task) {
    size_t outSize;
    gmd = readFile(state.customLevelPath, &outSize);
    
    if (gmd == NULL) return 1;

    gmd = convertIfGdlvl(gmd, state.customLevelPath);

    setNameCreator(gmd);
    setDescription(gmd);
    setDownloadsLikes(gmd);
    setStars(gmd);
    setDifficulty();
    setLevelID(gmd);
    setSongID(gmd);

    return 0;
}

static GenericTask task = {
    .func = loadGMD
};

static void externalPopupInitTop(UIScreen *s) {
    screenTop = s;
    levelName      = (UILabel *) uiGetElementByTag(screenTop, "levelname");
    creatorName    = (UILabel *) uiGetElementByTag(screenTop, "creatorname");
    description     = (UILabel *) uiGetElementByTag(screenTop, "description");
    downloadsLabel = (UILabel *) uiGetElementByTag(screenTop, "downloadcount");
    likesLabel     = (UILabel *) uiGetElementByTag(screenTop, "likecount");
    starsLabel     = (UILabel *) uiGetElementByTag(screenTop, "stars");
    difficultyFace = (UIImage *) uiGetElementByTag(screenTop, "difficultyface");
    levelIDLabel  = (UILabel *) uiGetElementByTag(screenTop, "levelid");
    likeImage      = (UIImage *) uiGetElementByTag(screenTop, "likeimage");
    uiRunFuncOnTag(screenTop, "info", uiDisableElement);
}

void showLevelLoadErrorMessage() {
    // Level gave error
    int messageID = levelResult - 1;
    const char *message = "Ultra unknown error.";
    if (IN_BOUNDS((size_t)messageID, errorStrings)) {
        message = (char *) errorStrings[messageID]; 
    }

    InfoCardData *extErrorData = (InfoCardData *)malloc(sizeof(InfoCardData));
    if(!extErrorData) return;

    extErrorData->text = strdup(message);
    extErrorData->copied = true;
    extErrorData->title = strdup("Error");
    extErrorData->customTitle = true;

    uiStackPush(&infoCardDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    uiStackPushData(extErrorData);

    levelResult = 0;
}

void externalPopupUpdate(UIScreen *s, UIInput *u) {
    if (levelResult) {
        showLevelLoadErrorMessage();
    }

    if (exitingLevel) {
        setProgress();
        exitingLevel = false;
    }
}

void externalPopupInit(UIScreen *s) {
    screen = s;
    songLabel            = (UILabel *) uiGetElementByTag(screen, "songid");
    normalProgress       = (UIProgressBar *) uiGetElementByTag(screen, "normalprogress");
    normalProgressVal   = (UILabel *) uiGetElementByTag(screen, "normalprogressvalue");
    practiceProgress     = (UIProgressBar *) uiGetElementByTag(screen, "practiceprogress");
    practiceProgressVal = (UILabel *) uiGetElementByTag(screen, "practiceprogressvalue");

    uiProgressBarSetTint(normalProgress, C2D_Color32(0, 255, 0, 255));
    uiProgressBarSetTint(practiceProgress, C2D_Color32(0, 255, 255, 255));

    thread = createGenericThread(&task);
    uiRunFuncOnTag(screen, "info", uiDisableElement);

    free(gmd);
}

void externalPopupUpdateTop(UIScreen *s, UIInput *u) {
    if (task.finished) {
        uiRunFuncOnTag(screen, "spinner", uiDisableElement);
        uiRunFuncOnTag(screen, "info", uiEnableElement);
        uiRunFuncOnTag(screenTop, "info", uiEnableElement);
        uiRunFuncOnTag(screenTop, "spinner", uiDisableElement);

        task.finished = false;
    }
}

extern const UIScreenDefPair externalPopupDef = {
    .name = "external_popup",
    .top = {
        .path = "romfs:/menus/creator_menu/external/external_pop_up_top.txt",
        .init = externalPopupInitTop,
        .update = externalPopupUpdateTop
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/external/external_pop_up.txt",
        .init = externalPopupInit,
        .update = externalPopupUpdate,
        .actionList = {
            .actions = externalPopupActions,
            .actionCount = ARRAY_LEN(externalPopupActions)
        }
    }
};

}