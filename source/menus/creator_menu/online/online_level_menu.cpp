#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include "3ds/thread.h"
#include "3ds/types.h"

#include "level_loading.hpp"
#include "main.hpp"
#include "menus/core/ui_element.hpp"
#include "mp3_player.hpp"
#include "save/saving.hpp"
#include "sounds.hpp"
#include "state.hpp"
#include "utils/folders.hpp"
#include "utils/server_utils.hpp"
#include "utils/string_helpers.hpp"
#include "utils/json_config.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/components/ui_list.hpp"
#include "menus/components/ui_image.hpp"
#include "menus/components/ui_label.hpp"
#include "menus/components/ui_button.hpp"
#include "menus/components/ui_progress_bar.hpp"

#include "menus/creator_menu/search_menu.hpp"
#include "menus/creator_menu/online/online_level_comments.hpp"
#include "menus/creator_menu/online/online_level_infobox.hpp"
#include "menus/creator_menu/online/two_option_pop_up.hpp"
#include "menus/creator_menu/online/online_menu.hpp"
#include "menus/creator_menu/external/external_popup.hpp"
#include "menus/creator_menu/online/online_level_comments.hpp"
#include "menus/settings_hub/info_card.hpp"

#include "fonts/chatFont.hpp"
#include "fonts/goldFont.hpp"
#include "utils/utils.hpp"
#include "wav_player.hpp"

#define EASY_DEMON_FACE_1 259
#define MEDIUM_DEMON_FACE_1 261
#define HARD_DEMON_FACE_1 257
#define INSANE_DEMON_FACE_1 263
#define EXTREME_DEMON_FACE_1 265

#define USER_COIN_CENTER_X      165.f
#define USER_COIN_STEP_X        9.f
#define USER_COIN_ROW_Y_RATED   131.f
#define USER_COIN_ROW_Y_UNRATED 123.f

namespace gd3ds
{

const int demonFaces1[] = {
    NA_FACE,
    EASY_DEMON_FACE_1,
    MEDIUM_DEMON_FACE_1,
    HARD_DEMON_FACE_1,
    INSANE_DEMON_FACE_1,
    EXTREME_DEMON_FACE_1
};

const int demonFaceFeaturedOffsets[] = {
    0,
    -9,
    -8,
    -8,
    -8,
    -8
};

static bool hasSavedLevel = false;
static char *loaded_level_string = NULL;
int onlineMenuLevelID = 0;
SearchEntry *currentSearchEntry = NULL;
CreatorEntry *currentCreatorEntry = NULL;
SongEntry *currentSongEntry = NULL;

int getSavedLevel(GenericTask *task) {
    size_t outSize;
    if (loaded_level_string && loaded_level_string != currLevelString) {
        free(loaded_level_string);
    }
    loaded_level_string = loadSavedLevel(onlineMenuLevelID, gdps, &outSize);
    return loaded_level_string == NULL;
}

static GenericTask levelTask = {
    .func = getLevel
};

static Thread levelThread;

static GenericTask songDataTask = {
    .func = getSongData
};

static Thread songDataThread;

static DownloadTask songTask = {
    .path = USER_SONGS_DIR,
    .extension = "mp3"
};

static Thread savedLevelThread;

static GenericTask savedLevelTask = {
    .func = getSavedLevel
};

static Thread songThread;

static bool alreadyPlayedOnlineLevel = false;

int result = -2;

bool refresh = false;
bool songExists = false;

int warningStep = 0;

char onlineLevelDownloadSpeed[24] = "Speed: 0 B/s";

static UIScreen *screenTop;
static UIScreen *screen;

static UILabel *levelNameLabel;
static UILabel *levelCreatorLabel;
static UIImage *highObjIconImage;
static UIImage *collabIconImage;
static UILabel *downloadsLabel;
static UIImage *likesImage;
static UILabel *likesLabel;
static UILabel *lengthLabel;
static UILabel *starsLabel;
static UILabel *descriptionLabel;
static UILabel *levelIDLabel;
static UIImage *difficultyFaceImage;
static UIImage *featuredGlowImage;
static UIImage *usercoinSlots[3];

static UIProgressBar *normalPercentProg;
static UIProgressBar *practicePercentProg;

static UISpinner *spinner;
static UIButton *playButton;
static UILabel *normalPercentLabel;
static UILabel *practicePercentLabel;
static UILabel *songNameLabel;
static UILabel *songArtistLabel;
static UILabel *songStatusLabel;
static UIProgressBar *songProgressBar;
static UILabel *songIDLabel;
static UILabel *speedLabel;
static UILabel *songSizeLabel;
static UIButton *songDownloadButton;

bool inWarning = false;
bool inWarningScreen = false;

typedef enum WarningPopupStep {
    WARNING_HIGH_OBJECT,
    WARNING_VERSION,
    WARNING_MISSING_SONG,
    WARNING_DONE
} WarningPopupStep;

const char* warningTitles[] = {
    "High objects",
    "Version warning",
    "Missing song"
};

const char* warnings[] = {
    "This level has a <#ffa54b>high object</> count\nand might not be <#ff5a5a>fully playable</>.",
    "This level was made or updated in an\n<#ffa54b>incompatible game version</>. It might\nnot be <#ff5a5a>fully playable</>.",
    "This level uses a <#4c8cc7>custom song</> that\nhas not been <#36c244>downloaded</> yet. Play\nwithout music?"
};

static void updateDownloadButton(SearchEntry *entry){
    bool songExists = checkSong(entry->songId);
    if (songExists) {
        uiDisableElement((UIElement *)songDownloadButton);
    }
}

static void actionDownload(UIElement *e, const UIPropertyList *args){
    if (!songTask.running && !songDataTask.running) {
        songProgressBar->value = 0;
        uiButtonSetImage(songDownloadButton, 22, 0);
        uiDisableElement((UIElement *) songStatusLabel);
        uiDisableElement((UIElement *) songIDLabel);
        uiEnableElement((UIElement *) songProgressBar);
        uiEnableElement((UIElement *) speedLabel);
        snprintf(onlineLevelDownloadSpeed, sizeof(onlineLevelDownloadSpeed), "Speed: 0 B/s");
        uiLabelSetText(speedLabel, onlineLevelDownloadSpeed);
        songDataThread = createGenericThread(&songDataTask);
    } else {
        if (songDataTask.running) {
            songDataTask.cancelled = true;
            threadJoin(songDataThread, U64_MAX);
        }

        if (songTask.running) {
            songTask.cancelled = true;
            threadJoin(songThread, U64_MAX);
        }
    }
}

static void openWarning(){
    const char *warning = "Ultra unknown error.";
    if (IN_BOUNDS((size_t)warningStep, warnings)) {
        warning = (const char *) warnings[warningStep]; 
    } else return;

    const char *warningTitle = "What?";
    if (IN_BOUNDS((size_t)warningStep, warningTitles)) {
        warningTitle = (const char *) warningTitles[warningStep]; 
    } else return;

    TwoOptionPopupData *warningData = (TwoOptionPopupData *)malloc(sizeof(TwoOptionPopupData));
    if(!warningData) return;

    warningData->text = strdup(warning);
    warningData->title = strdup(warningTitle);
    warningData->proceedText = strdup("Play");
    warningData->cancelText = strdup("Cancel");

    uiStackPush(&warningPopUpDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    uiStackPushData(warningData);
}

static void playLevel() {
    playSfx(&playSound, 1);

    state.customLevel = true;
    state.onlineLevel = true;

    alreadyPlayedOnlineLevel = true;

    stopMP3();
    playingMenuLoop = false;


    if (hasSavedLevel) {
        currLevelString = loaded_level_string;
    } else {
        currLevelString = levelEntry->levelString;
    }

    uiStackPushGameState(STATE_GAME);
}

void checkWarningsAndPlay(){
    if(result != 0) return;

    if(!alreadyPlayedOnlineLevel){
        bool shouldWarn;

        while(warningStep <= (int)WARNING_DONE){
            switch(warningStep){
                case WARNING_HIGH_OBJECT: {
                    int objCount = (isN3Ds ? 44000 : 14000);
                    shouldWarn = currentSearchEntry->objCount >= objCount;
                    break;
                }
                case WARNING_VERSION: {
                    shouldWarn = deriveGJVersion(currentSearchEntry->gameVersion) > GD_VERSION;
                    break;
                }
                case WARNING_MISSING_SONG: {
                    int songID = currentSearchEntry->songId;
                    shouldWarn = !(songID == 0 || checkSong(songID));
                }
                default:
                    break;
            }

            if(shouldWarn) break;
            warningStep++;
        }

        if(shouldWarn){
            openWarning();
            warningStep++;
            return;
        }
    }

    playLevel();
}

static void actionPlay(UIElement *e, const UIPropertyList *args) {
    checkWarningsAndPlay();
}

static void actionOpenInfo(UIElement *e, const UIPropertyList *args) {
    if (result == 0 || alreadyPlayedOnlineLevel){
        uiStackPush(&onlineInfoboxDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    }
}

void deleteLevel(){
   removeSavedLevel(onlineMenuLevelID, gdps);
}

static void actionOpenDeleteLevel(UIElement *e, const UIPropertyList *args){
    if (result == 0 || alreadyPlayedOnlineLevel){
        TwoOptionPopupData *deleteLevelData = (TwoOptionPopupData *)malloc(sizeof(TwoOptionPopupData));
        if(!deleteLevelData) return;

        deleteLevelData->text = strdup("Are you sure you want to\ndelete this level?");
        deleteLevelData->title = strdup("Delete Level");
        deleteLevelData->proceedText = strdup("Yes");
        deleteLevelData->cancelText = strdup("No");

        uiStackPush(&deletePopUpDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
        uiStackPushData(deleteLevelData);
    }
}

static void updateProgressBars() {
    LevelData *data = &currentLevelEntry->data;
    normalPercentProg->value = data->normalProgress;
    practicePercentProg->value = data->practiceProgress;

    char normal[16];
    char practice[16];
    snprintf(normal, sizeof(normal), "%d%%", data->normalProgress);
    snprintf(practice, sizeof(practice), "%d%%", data->practiceProgress);

    uiLabelSetText(normalPercentLabel, normal);
    uiLabelSetText(practicePercentLabel, practice);
}

static void populateLevelInfo(int levelID) {
    SearchEntry *entrySrch = currentSearchEntry;
    CreatorEntry *entryC = currentCreatorEntry;
    SongEntry *entrySng = currentSongEntry;

    char *downloads = truncateNumber(entrySrch->downloads);
    uiLabelSetText(downloadsLabel, downloads);

    char *likes = truncateNumber(entrySrch->likes);
    uiLabelSetText(likesLabel, likes);

    if (entrySrch->likes < 0) {
        uiImageSetImage(likesImage, DISLIKE_ICON, 0);
    }

    // Length
    const char *length = "Unkn.";
    if (IN_BOUNDS((size_t)entrySrch->lengthNum, levelLengths)) {
        length = (const char *) levelLengths[entrySrch->lengthNum];
    }
    uiLabelSetText(lengthLabel, length);

    // Level ID
    char lvlid[32];
    snprintf(lvlid, sizeof(lvlid), "<#78aaf0>ID: %d", levelID);
    uiLabelSetText(levelIDLabel, lvlid);

    // Creator
    char creator[36] = "<#5AFFFF>By -</>";
    if (entryC) snprintf(creator, sizeof(creator), "<#%s>By %s</>", (entryC->userId == 0) ? "5AFFFF" : "FFFFFF", entryC->creatorName);
    uiLabelSetText(levelCreatorLabel, creator);

    // Song and song artist
    const char *songName = "Unknown";
    const char *songArtistName = "Unknown";

    if (entrySrch->songId != 0) {
        // Custom song
        char tmpSongsize[24];
        snprintf(tmpSongsize, sizeof(tmpSongsize), "Size: %.1fMB", entrySng->songSize);
        uiLabelSetText(songSizeLabel, tmpSongsize);

        if (currentSongEntry) {
            songName = entrySng->songTitle;
            songArtistName = entrySng->artistName;
        }
        updateDownloadButton(entrySrch);
    } else {
        // Main level song
        if (entrySrch->mainSongId >= 0 && entrySrch->mainSongId < currentMainLevelPack->count) {
            songName = (const char *) currentMainLevelPack->levels[entrySrch->mainSongId].songData.title;
            songArtistName = (const char *) currentMainLevelPack->levels[entrySrch->mainSongId].songData.artist;
        }
        
        uiDisableElement((UIElement *)songSizeLabel);
        uiDisableElement((UIElement *)songDownloadButton);
    }

    // Song artist again
    char songArtist[132];
    snprintf(songArtist, sizeof(songArtist), "By: %s", songArtistName);
    uiLabelSetText(songArtistLabel, songArtist);
    uiLabelSetText(songNameLabel, songName);
    
    // Star count
    char starCount[4];
    snprintf(starCount, sizeof(starCount), "%d", entrySrch->stars);
    uiLabelSetText(starsLabel, starCount);

    // Unrated
    if (entrySrch->stars == 0) {
        uiRunFuncOnTag(screenTop, "star", uiDisableElement);
        uiElementSetPosition((UIElement *) featuredGlowImage, 165, 97.65);
        uiElementSetPosition((UIElement *) difficultyFaceImage, 165, 93);
    }

    uiLabelSetText(levelNameLabel, entrySrch->name);
    
    // Set difficulty
    int difficulty_id = NA_FACE;

    int featuredDemonOffset = 0;

    if(entrySrch->isAuto) {
        difficulty_id = AUTO_FACE;
    } else if(entrySrch->isDemon && gdps) {
        difficulty_id = 258;
    } else if (entrySrch->isDemon && IN_BOUNDS((size_t)entrySrch->difficulty, demonFaces1)) {
        difficultyFaceImage->base.y = 87 - 5;
        featuredDemonOffset = demonFaceFeaturedOffsets[entrySrch->difficulty];
        difficulty_id = demonFaces1[entrySrch->difficulty];
    } else if (!entrySrch->isDemon && IN_BOUNDS((size_t)entrySrch->difficulty, difficultyFaces)) {
        difficulty_id = difficultyFaces[entrySrch->difficulty];
    }

    uiImageSetImage(difficultyFaceImage, difficulty_id, 0);

    if (entrySrch->featureScore > 0) {
        int featuredID = 0;
        int yOffset = 0;

        if(entrySrch->epic > 0 && IN_BOUNDS((size_t)entrySrch->epic, epics)){
            featuredID = (gdps ? SUPER_GLOW : epics[entrySrch->epic]);
            yOffset = -2;
        } else if(entrySrch->featureScore > 0) {
            featuredID = FEATURED_GLOW;
        }

        if(!entrySrch->stars) yOffset += 4;

        if(entrySrch->isDemon) yOffset += featuredDemonOffset;

        featuredGlowImage->base.x = 165 + (gdps ? 0.3 : 0);
        featuredGlowImage->base.y = 81.65 + yOffset;

        uiImageSetImage(featuredGlowImage, featuredID, 0);
    } else{
        uiDisableElement((UIElement *)featuredGlowImage);
    }

    // Description
    char *wrappedDescription = wrapText(&chatFontFontCharset, descriptionLabel->base.scaleX, entrySrch->description, 270);
    char *desc = strdup(wrappedDescription);
    uiLabelSetText(descriptionLabel, desc);
    free(desc);

    // Song id
    char songID[16];
    snprintf(songID, sizeof(songID), "SongID: %d", (entrySrch->songId == 0) ? entrySrch->mainSongId + 1 : entrySng->ngSongId);
    uiLabelSetText(songIDLabel, songID );

    // Level icons
    float halfCreatorLength = getTextLength(&goldFontFontCharset, 0.7f, false, (entryC ? entryC->creatorName : "-")) / 2;

    // Original icon
    bool isCopy = entrySrch->originalId != 0;
    if (isCopy) {
        uiElementSetPosition((UIElement *)collabIconImage, 200 + halfCreatorLength + 24, collabIconImage->base.y);
    } else {
        uiDisableElement((UIElement *)collabIconImage);
    }

    // High object count icon
    bool highObjCount = entrySrch->objCount >= (isN3Ds ? 44000 : 14000);
    if (highObjCount) {
        uiElementSetPosition((UIElement *)highObjIconImage, 200 + halfCreatorLength + 24 + (isCopy ? 13 : 0), highObjIconImage->base.y);
    } else {
        uiDisableElement((UIElement *)highObjIconImage);
    }
    
    char key[16];
    snprintf(key, sizeof(key), "%d", entrySrch->levelId);
    
    currentLevelEntry = getOrAddLevelToServerFile(currentServerFile, key, LEVEL_LIST_ONLINE);
    currentLevelList = LEVEL_LIST_ONLINE;

    currentLevelEntry->data.levelID = entrySrch->levelId;
    currentLevelEntry->data.stars = entrySrch->stars;

    int coinCount = entrySrch->coins;
    if (coinCount < 0) coinCount = 0;
    if (coinCount > 3) coinCount = 3;
    bool coinsVisible = userCoinsCounterVisible();
    bool hasVerifiedCoins = entrySrch->stars > 0 && entrySrch->verifiedCoins;
    levelVerifiedCoins = entrySrch->verifiedCoins;
    float coinRowY = entrySrch->stars > 0 ? USER_COIN_ROW_Y_RATED : USER_COIN_ROW_Y_UNRATED;
    for (int i = 0; i < 3; i++) {
        UIImage *slot = usercoinSlots[i];
        if (!slot) continue;
        if (coinsVisible && i < coinCount) {
            float coinX = USER_COIN_CENTER_X + (i - (coinCount - 1) / 2.f) * USER_COIN_STEP_X;
            bool collected = currentLevelEntry &&
                (i == 0 ? currentLevelEntry->data.coin1 :
                 i == 1 ? currentLevelEntry->data.coin2 :
                          currentLevelEntry->data.coin3);
            uiElementSetPosition((UIElement *) slot, coinX, coinRowY);
            uiImageSetTint(slot, userCoinIconTint(collected, hasVerifiedCoins));
            slot->base.enabled = true;
        } else {
            slot->base.enabled = false;
        }
    }

    updateProgressBars();
}

static void handleErrors(int code) {
    uiDisableElement((UIElement *) spinner);
    char errorMessage[64];
    switch (code) {
        case -2: 
            snprintf(errorMessage, sizeof(errorMessage), "%s", "An unknown error has occured.");
            break;
        case -1:
            snprintf(errorMessage, sizeof(errorMessage), "%s", "Failed to download level!\nPlease try again later.");
            break;
        case 6:
        case 7:
            snprintf(errorMessage, sizeof(errorMessage), "No <#41e24e>Internet</> connection!");
            break;
        case 28:
            snprintf(errorMessage, sizeof(errorMessage), "Connection timed out!");
            break;
        case 42:
            break;
        default:
            snprintf(errorMessage, sizeof(errorMessage), "An unknown error has occurred.\nError code: %d", code);
            break;
    }
    InfoCardData *data = (InfoCardData *)malloc(sizeof(InfoCardData));
    if(!data) return;

    data->text = strdup(errorMessage);
    data->title = strdup("Error");
    data->customTitle = true;
    data->copied = true;

    uiStackPush(&infoCardDef, ANIM_ZOOM, ANIM_ZOOM, PUSH_NEXT);
    uiStackPushData(data);
}

static void handleSongCodes(int code) {
    uiDisableElement((UIElement *) songProgressBar);
    uiDisableElement((UIElement *) speedLabel);
    uiEnableElement((UIElement *) songIDLabel);
    uiEnableElement((UIElement *) songStatusLabel);
    uiButtonSetImage(songDownloadButton, 57, 0);
    char message[64];
    switch (code) {
        case -4:
            snprintf(message, sizeof(message), "<#f93219>Download failed.</>");
            break;
        case -3:
        case -2: 
            snprintf(message, sizeof(message), "<#f93219>Unknown error.</>");
            break;
        case 0:
            snprintf(message, sizeof(message), "<#00FF00>Download complete.</>");
            uiDisableElement((UIElement *) songDownloadButton);
            break;
        case 6:
        case 7:
            snprintf(message, sizeof(message), "<#f93219>No Internet connection!</>");
            break;
        case 42:
            snprintf(message, sizeof(message), "<#f93219>Download cancelled.</>");
            break;
        case 28:
            snprintf(message, sizeof(message), "Connection timed out!");
            break;
        default:
            snprintf(message, sizeof(message), "<#f93219>Unknown error. Code: %d</>", code);
            break;
    }
    uiLabelSetText(songStatusLabel, message);
}

static void handleSongDataErrors(int code) {
    uiDisableElement((UIElement *) songProgressBar);
    uiDisableElement((UIElement *) speedLabel);
    uiEnableElement((UIElement *) songStatusLabel);
    uiEnableElement((UIElement *) songIDLabel);
    uiButtonSetImage(songDownloadButton, 57, 0);
    char message[64] = "";
    switch (code) {
        case -3: 
            snprintf(message, sizeof(message), "<#f93219>Unknown data failure.</>");
            break;
        case -2:
            snprintf(message, sizeof(message), "<#f93219>Song not allowed for use.</>");
            break;
        case -1: 
            snprintf(message, sizeof(message), "<#f93219>Failed to fetch info.</>");
            break;
        case 6:
        case 7:
            snprintf(message, sizeof(message), "<#f93219>No Internet connection!</>");
            break;
        case 42:
            snprintf(message, sizeof(message), "<#f93219>Download cancelled.</>");
            break;
        case 28:
            snprintf(message, sizeof(message), "Connection timed out!");
            break;
        default:
            snprintf(message, sizeof(message), "<#f93219>Unknown error. Code: %d</>", result);
            break;
    }
    uiLabelSetText(songStatusLabel, message);
}

static void actionRefreshLevel(UIElement *e, const UIPropertyList *props) {
    result = -2;
    refresh = true;
    uiEnableElement((UIElement *)spinner);
    uiDisableElement((UIElement *)playButton);
    levelThread = createGenericThread(&levelTask);
}

static UIActionDef onlineLevelActions[] = {
    {"info", actionOpenInfo },
    {"delete", actionOpenDeleteLevel},
    {"reload", actionRefreshLevel },
    {"play", actionPlay },
    {"download", actionDownload },
};

static void onlineLevelInitTop (UIScreen *s) {
    screenTop = s;
    // Top screen elements
    levelNameLabel = (UILabel *) uiGetElementByTag(screenTop, "levelname");
    levelCreatorLabel = (UILabel *) uiGetElementByTag(screenTop, "creatorname");

    collabIconImage = (UIImage *) uiGetElementByTag(screenTop, "collab");
    highObjIconImage = (UIImage *) uiGetElementByTag(screenTop, "highobj");

    downloadsLabel = (UILabel *) uiGetElementByTag(screenTop, "downloadcount");
    likesLabel = (UILabel *) uiGetElementByTag(screenTop, "likecount");
    lengthLabel = (UILabel *) uiGetElementByTag(screenTop, "length");
    starsLabel = (UILabel *) uiGetElementByTag(screenTop, "starcount");

    difficultyFaceImage = (UIImage *) uiGetElementByTag(screenTop, "difficultyface");
    featuredGlowImage = (UIImage *) uiGetElementByTag(screenTop, "glow");

    usercoinSlots[0] = (UIImage *) uiGetElementByTag(screenTop, "usercoin_slot_1");
    usercoinSlots[1] = (UIImage *) uiGetElementByTag(screenTop, "usercoin_slot_2");
    usercoinSlots[2] = (UIImage *) uiGetElementByTag(screenTop, "usercoin_slot_3");

    likesImage = (UIImage *) uiGetElementByTag(screenTop, "thumbsup");

    descriptionLabel = (UILabel *) uiGetElementByTag(screenTop, "description");
    levelIDLabel = (UILabel *) uiGetElementByTag(screenTop, "levelid");
}

static void onlineLevelInit (UIScreen *s) {
    commentsNeedRefresh = true;
    warningStep = 0;
    result = -2;
    refresh = false;

    screen = s;

    if(gdps) uiDisableElement(uiGetElementByTag(s, "garage"));

    // Bottom screen elements
    normalPercentProg = (UIProgressBar *) uiGetElementByTag(screen, "normalprogress");
    practicePercentProg = (UIProgressBar *) uiGetElementByTag(screen, "practiceprogress");
    normalPercentLabel = (UILabel *) uiGetElementByTag(screen, "normalprogressvalue");
    practicePercentLabel = (UILabel *) uiGetElementByTag(screen, "practiceprogressvalue");

    songNameLabel = (UILabel *) uiGetElementByTag(screen, "songname");
    songArtistLabel = (UILabel *) uiGetElementByTag(screen, "songartist");
    songIDLabel = (UILabel *) uiGetElementByTag(screen, "songid");
    speedLabel = (UILabel *) uiGetElementByTag(screen, "speed");
    songSizeLabel = (UILabel *) uiGetElementByTag(screen, "songsize");
    songDownloadButton = (UIButton *) uiGetElementByTag(screen, "downloadbtn");
    songStatusLabel = (UILabel *) uiGetElementByTag(screen, "songstatus");
    songProgressBar = (UIProgressBar *) uiGetElementByTag(screen, "progressbar");

    spinner = (UISpinner *) uiGetElementByTag(screen, "spinner");
    playButton = (UIButton *) uiGetElementByTag(screen, "playbutton");

    uiProgressBarSetTint(normalPercentProg, C2D_Color32(0, 255, 0, 255));
    uiProgressBarSetTint(practicePercentProg, C2D_Color32(0, 255, 255, 255));
    uiProgressBarSetTint(songProgressBar, C2D_Color32(50, 190, 240, 255));

    uiDisableElement((UIElement *) songProgressBar);
    uiDisableElement((UIElement *) songStatusLabel);
    uiDisableElement((UIElement *) speedLabel);

    SavedLevelDataEntry *data = getSavedLevelData(onlineMenuLevelID);
    if (data && !refresh && !redownload) {
        currentSearchEntry = &data->searchEntry;
        currentCreatorEntry = &data->creatorEntry;
        currentSongEntry = &data->songEntry;
    } else {
        currentSearchEntry = &searchEntries[currSearchID];
        int creatorIndex = currentSearchEntry->creatorIndex;
        currentCreatorEntry = (creatorIndex >= 0 ? &creatorEntries[creatorIndex] : NULL);
        currentSongEntry = (currentSearchEntry->songId >= 0 ? &songEntries[currentSearchEntry->songIndex] : NULL);
        bool couldSave = saveLevelToServerFile(currentServerFile, onlineMenuLevelID, currentSearchEntry, currentCreatorEntry, currentSongEntry);
        if (!couldSave) {
            outputLog("Oops, couldn't save da level!\n");
        }
    }

    playMenuSong();
    
    if (!alreadyPlayedOnlineLevel) {
        uiDisableElement((UIElement *)playButton);
    } else {
        uiDisableElement((UIElement *)spinner);
        uiEnableElement((UIElement *)playButton);
    }

    populateLevelInfo(onlineMenuLevelID);

    if (!alreadyPlayedOnlineLevel) {
        levelEntry = NULL;
        if (savedLevelExists(onlineMenuLevelID, gdps) && !redownload) {
            hasSavedLevel = true;
            savedLevelThread = createGenericThread(&savedLevelTask);
        } else {
            hasSavedLevel = false;
            levelThread = createGenericThread(&levelTask);
        }
    }
}

static void onlineLevelMenuUpdate(UIScreen *s, UIInput *i) {
    if (levelResult) {
        showLevelLoadErrorMessage();
    }

    if (exitingLevel) {
        updateProgressBars();
        exitingLevel = false;
    }

    if (songDataTask.finished) {
        int songDataResult = -3;
        songDataResult = songDataTask.result;
        // Handle result
        if (songDataResult == 0) {
            songDataTask.finished = false;
            snprintf(songTask.fileName, sizeof(songTask.fileName), "%d", currentSearchEntry->songId);
            songTask.url = currentSongEntry->songLink;

            // enter exclusive mode, disable sleep mode and begin download 
            aptSetSleepAllowed(false);
            enterExclusiveState();
            songThread = createFileDownloadThread(&songTask);
        } else { handleSongDataErrors(songDataResult); }
    }

    if (songTask.running) {
        songProgressBar->value = songTask.progress;
        
        char *speed = truncateSpeed(songTask.speed);
        if (speed) {
            snprintf(onlineLevelDownloadSpeed, sizeof(onlineLevelDownloadSpeed), "Speed: %s", speed);
            uiLabelSetText(speedLabel, onlineLevelDownloadSpeed);
            free(speed);
        }
    }

    // Run when finished
    if (songTask.finished) {
        // Handle result
        handleSongCodes(songTask.result);
        exitExclusiveState(false);
        aptSetSleepAllowed(true);
        songTask.finished = false;
    }

    // Run when finished
    if (levelTask.finished) {
        result = levelTask.result;
        // Handle result
        if (result != 0) {
            handleErrors(result);
            if (hasSavedLevel) {
                result = 0;
                uiEnableElement((UIElement *) playButton);
            }
        } else { // No errors
            uiDisableElement((UIElement *) spinner);
            uiEnableElement((UIElement *) playButton);
            if (refresh == true) {
                populateLevelInfo(onlineMenuLevelID);
                refresh = false;
            }
        }
        levelTask.finished = false;
    }

    if (savedLevelTask.finished) {
        result = savedLevelTask.result;
        alreadyPlayedOnlineLevel = true;
        uiDisableElement((UIElement *) spinner);
        uiEnableElement((UIElement *) playButton);
        savedLevelTask.finished = false;
    }
}

static void onlineLevelMenuExit(UIScreen *s) {
    if (levelEntry) {
        if (levelEntry->levelString && !isLevelArenaPtr(levelEntry->levelString))
            free(levelEntry->levelString);
        free(levelEntry);
        levelEntry = NULL;
    }

    if (loaded_level_string && loaded_level_string != currLevelString) {
        free(loaded_level_string);
    }
    loaded_level_string = NULL;
    hasSavedLevel = false;
    
    if (commentEntries) {
        free(commentEntries);
        commentEntries = NULL;
    }

    if (levelTask.running) {
        levelTask.cancelled = true;
        threadJoin(levelThread, U64_MAX);
    }

    if (songDataTask.running) {
        songDataTask.cancelled = true;
        threadJoin(songDataThread, U64_MAX);
    }

    if (songTask.running) {
        songTask.cancelled = true;
        exitExclusiveState(false);
        aptSetSleepAllowed(true);
        threadJoin(songThread, U64_MAX);
    }

    alreadyPlayedOnlineLevel = false;
    saveCurrentSaveFile(LEVEL_LIST_ONLINE);
}

extern const UIScreenDefPair onlineLevelMenuDef = {
    .name = "online_level_menu",
    .top = {
        .path = "romfs:/menus/creator_menu/online/online_level_menu_top.txt",
        .init = onlineLevelInitTop
    },
    .btm = {
        .path = "romfs:/menus/creator_menu/online/online_level_menu.txt",
        .init = onlineLevelInit,
        .update = onlineLevelMenuUpdate,
        .exit = onlineLevelMenuExit,
        .actionList = {
            .actions = onlineLevelActions,
            .actionCount = ARRAY_LEN(onlineLevelActions)
        }
    }
};

}