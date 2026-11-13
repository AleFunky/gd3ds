#include "level_loading.hpp"
#include "main.hpp"
#include "sounds.hpp"
#include "state.hpp"
#include "math_helpers.hpp"
#include "mp3_player.hpp"
#include "particles/circles.hpp"

#include "endwall.hpp"
#include "particles/rays.hpp"
#include "particles/circles.hpp"

#include "menus/core/ui_screen.hpp"
#include "menus/level_complete.hpp"
#include "practice.hpp"
#include "menus/level_select.hpp"
#include "wav_player.hpp"

namespace gd3ds
{

static int fireworksSpawned = 0;
static int circunferencesSpawned = 0;
static float circunferenceTimer = 0.0f;
static bool levelCompleteInitialized = false;
static float completionTimer = 0.0f;

#include "save/saving.hpp"

typedef enum {
    SCALE_IN,
    WAITING,
    SCALE_OUT,
} LevelCompleteState;

typedef struct {
    bool active;
    
    LevelCompleteState state;

    float scale;

    float initialScale;
    float targetScale;
    float timer;
    float duration;
    EaseTypes ease;
} LevelCompletePopup;

static void initLevelCompletePopup();

static LevelCompletePopup levelCompletePopup;

int handleWallCutscene(float delta, UIInput *touch) {
    if(!exitingLevel){
        bool practiceModeEndWall = state.practiceMode || cheated;
        // Init wall variables
        if (completionTimer == 0) {
            fireworksSpawned = 0;
            circunferencesSpawned = 0;

            // Skip rays and co
            if (practiceModeEndWall) {
                completionTimer = FIREWORK_SPAWN_TIME;
                return 0;
            }

            startShake(FIREWORK_SPAWN_TIME, 3);
            
            UseEffect *effect = addUseEffect(levelInfo.wallX, levelInfo.wallY, USE_EFFECT_OBJ_NOTHING, &endWallFilledFirst, getUseEffectArrayPtr(GFX_TOP));
            if (effect) {
                Color p1White = getWhiteIfBlack(p1Color);
                effect->def.colorR = p1White.r / 255.f;
                effect->def.colorG = p1White.g / 255.f;
                effect->def.colorB = p1White.b / 255.f;
            }
            
            playSfx(&endSound, 1);

            raysStart();
        }

        // Make circunferences
        if (circunferenceTimer >= (circunferencesSpawned * CIRCUNFERENCE_SPAWN_DELAY) && circunferencesSpawned < CIRCUNFERENCE_COUNT && !practiceModeEndWall) {        
            UseEffect *effect = addUseEffect(levelInfo.wallX, levelInfo.wallY, USE_EFFECT_OBJ_NOTHING, &endWallCircunference, getUseEffectArrayPtr(GFX_TOP));
            if (effect) {
                Color p1White = getWhiteIfBlack(p1Color);
                effect->def.colorR = p1White.r / 255.f;
                effect->def.colorG = p1White.g / 255.f;
                effect->def.colorB = p1White.b / 255.f;
            }
            circunferencesSpawned++;
        }

        // Handle level complete text and co
        if (completionTimer >= FIREWORK_SPAWN_TIME && completionTimer <= MENU_TIME) {
            // Init circles
            if (fireworksSpawned == 0) {
                raysStartFade();

                initLevelCompletePopup();

                // Spawn again circunferences
                circunferencesSpawned = 0;
                circunferenceTimer = 0.0f;

                // Endwall circle
                UseEffect *effect = addUseEffect(levelInfo.wallX, levelInfo.wallY, USE_EFFECT_OBJ_NOTHING, &endWallFilledSecond, getUseEffectArrayPtr(GFX_TOP));
                if (effect) {
                    Color p1White = getWhiteIfBlack(p1Color);
                    effect->def.colorR = p1White.r / 255.f;
                    effect->def.colorG = p1White.g / 255.f;
                    effect->def.colorB = p1White.b / 255.f;
                }

                // Pop up circle
                float calcX = state.cameraX + (SCREEN_WIDTH_AREA / 2);
                float calcY = state.cameraY + (SCREEN_HEIGHT - (SCREEN_HEIGHT_AREA / 2));
                UseEffect *effect2 = addUseEffect(calcX, calcY, USE_EFFECT_OBJ_NOTHING, &endWallFilledTitle, getUseEffectArrayPtr(GFX_TOP_BUT_ABOVE_LEVEL));
                if (effect2) {
                    Color p1White = getWhiteIfBlack(p1Color);
                    effect2->def.colorR = p1White.r / 255.f;
                    effect2->def.colorG = p1White.g / 255.f;
                    effect2->def.colorB = p1White.b / 255.f;
                }

                levelCompleteEffectP1.emitterX = SCREEN_WIDTH_AREA / 2;
                levelCompleteEffectP1.emitterY = SCREEN_HEIGHT_AREA / 2;
                levelCompleteEffectP2.emitterX = SCREEN_WIDTH_AREA / 2;
                levelCompleteEffectP2.emitterY = SCREEN_HEIGHT_AREA / 2;

                spawnMultipleParticles(&levelCompleteEffectP1, 200);
                spawnMultipleParticles(&levelCompleteEffectP2, 200);
                
                if (practiceModeEndWall) fireworksSpawned++;
            }

            // Fireworks
            if (!practiceModeEndWall && completionTimer >= FIREWORK_SPAWN_TIME + (fireworksSpawned * FIREWORK_SPAWN_DELAY)) {
                float calcX = state.cameraX + 100 + randomFloat(0, SCREEN_WIDTH_AREA - 200);
                float y = randomFloat(0, SCREEN_HEIGHT_AREA);
                float calcY = state.cameraY + (SCREEN_HEIGHT - y);
                UseEffect *effect = addUseEffect(calcX, calcY, USE_EFFECT_OBJ_NOTHING, &endWallFireworkCircle, getUseEffectArrayPtr(GFX_TOP_BUT_ABOVE_LEVEL));
                if (effect) {
                    Color p2White = getWhiteIfBlack(p2Color);
                    effect->def.colorR = p2White.r / 255.f;
                    effect->def.colorG = p2White.g / 255.f;
                    effect->def.colorB = p2White.b / 255.f;
                }

                endWallFirework.emitterX = calcX;
                endWallFirework.emitterY = calcY;
                spawnMultipleParticles(&endWallFirework, 25);
                
                fireworksSpawned++;
            }
        }

        completionTimer += delta;
        circunferenceTimer += delta;
    }

    static int status = 0;
    
    // End the level!
    if (completionTimer > MENU_TIME) {
        if (!levelCompleteInitialized) {
            levelCompleteInit();
            levelCompleteInitialized = true;
        }

        if(status){
            LevelData *levelDataSel = &currentLevelEntry->data;
            if (!cheated) {
                if (state.practiceMode) {
                    state.currentData.maxPractice = 100;
                    levelDataSel->practiceProgress = 100;
                } else {
                    state.currentData.maxNormal = 100;
                    
                    // Add totals
                    if (levelDataSel->normalProgress < 100) {
                        int stars = 0;
                        if (state.customLevel) {
                            completedExternalLevels++;
                            stars = levelDataSel->stars;

                            // Add if demon
                            if (stars == 10) {
                                totalDemons++;
                            }
                        } else {
                            completedMainLevels++;
                            stars = currentMainLevelPack->levels[currLevelID].stars;

                            // Add if demon
                            if (currentMainLevelPack->levels[currLevelID].difficulty == MAIN_DIFF_DEMON) {
                                totalDemons++;
                            }
                        }

                        totalStars += stars;
                    }

                    if (state.onlineLevel) {
                        if (levelHasRate(levelDataSel)) {
                            if (!levelDataSel->coin1 && state.currentData.coin1) {
                                totalUserCoins++;
                            }
                            if (!levelDataSel->coin2 && state.currentData.coin2) {
                                totalUserCoins++;
                            }
                            if (!levelDataSel->coin3 && state.currentData.coin3) {
                                totalUserCoins++;
                            }
                        }
                    } else if (!state.customLevel) {
                        if (!levelDataSel->coin1 && state.currentData.coin1) {
                            totalCoins++;
                        }
                        if (!levelDataSel->coin2 && state.currentData.coin2) {
                            totalCoins++;
                        }
                        if (!levelDataSel->coin3 && state.currentData.coin3) {
                            totalCoins++;
                        }
                    }
                    
                    levelDataSel->normalProgress = 100;

                    levelDataSel->coin1 |= state.currentData.coin1;
                    levelDataSel->coin2 |= state.currentData.coin2;
                    levelDataSel->coin3 |= state.currentData.coin3;
                }
            }
            // Exiting
            if (status == 1) {
                stopMP3();
            } else if (status == 2) { // Restarting
                levelDataSel->attempts += state.currentData.attempts;
                levelDataSel->jumps += state.currentData.jumps;

                totalAttempts += state.currentData.attempts;
                totalJumps += state.currentData.jumps;

                firstLoadInitVariables();
                reloadLevel(); 
                if (state.practiceMode) {
                    clearPracticeMode();
                    stopMP3();
                    playLevelSong(levelInfo.songOffset);
                } else {
                    if (songLoaded) seekMP3(levelInfo.songOffset);
                }

                unpausePlaybackMP3();
            }
            levelCompleteInitialized = false;
            completionTimer = 0.0f;
            circunferenceTimer = 0.0f;
            return status;
        }
    }

    //level complete loop is done after init so it doesn't flicker for a frame
    status = levelCompleteLoop(touch);

    return 0;
}

static void initLevelCompletePopup() {
    levelCompletePopup.active = true;
    levelCompletePopup.state = SCALE_IN;
    levelCompletePopup.ease = ELASTIC_OUT;

    levelCompletePopup.timer = 0;
    levelCompletePopup.duration = LVL_COMPLETE_STATE_0_DURATION;
    levelCompletePopup.initialScale = 0.01;
    levelCompletePopup.targetScale = LVL_COMPLETE_STATE_0_TARGET_SCALE;
}

void clearLevelCompletePopup() {
    levelCompletePopup.active = false;
}

void handleLevelCompletePopup(float delta) {
    if (levelCompletePopup.active) {
        switch (levelCompletePopup.state) {
            case SCALE_IN:
                if (levelCompletePopup.timer > LVL_COMPLETE_STATE_0_DURATION) {
                    levelCompletePopup.state = WAITING;
                    levelCompletePopup.ease = EASE_LINEAR;

                    levelCompletePopup.timer = 0;
                    levelCompletePopup.duration = LVL_COMPLETE_STATE_1_DURATION;
                    levelCompletePopup.initialScale = levelCompletePopup.scale;
                    levelCompletePopup.targetScale = levelCompletePopup.scale;
                }
                break;
            case WAITING:
                if (levelCompletePopup.timer > LVL_COMPLETE_STATE_1_DURATION) {
                    levelCompletePopup.state = SCALE_OUT;
                    levelCompletePopup.ease = QUAD_IN;
                    
                    levelCompletePopup.timer = 0;
                    levelCompletePopup.duration = LVL_COMPLETE_STATE_2_DURATION;
                    levelCompletePopup.initialScale = levelCompletePopup.scale;
                    levelCompletePopup.targetScale = LVL_COMPLETE_STATE_2_TARGET_SCALE;
                }
                break;
            case SCALE_OUT:
                if (levelCompletePopup.timer > LVL_COMPLETE_STATE_2_DURATION) {
                    levelCompletePopup.active = false;
                }
                break;
        }
        levelCompletePopup.scale = easeValue(levelCompletePopup.ease, levelCompletePopup.initialScale, levelCompletePopup.targetScale, levelCompletePopup.timer, levelCompletePopup.duration, 1.f);

        levelCompletePopup.timer += delta;
    }
}

void drawLevelCompletePopup() {
    if (levelCompletePopup.active) {
        float scale = levelCompletePopup.scale;

        C2D_Sprite text = { 0 };
        C2D_SpriteFromSheet(&text, uiSheet, (state.practiceMode ? LVL_COMPLETE_PRACTICE_IMAGE_ID : LVL_COMPLETE_IMAGE_ID));
        C3D_TexSetFilter(text.image.tex, GPU_LINEAR, GPU_LINEAR);
        C2D_SpriteSetCenter(&text, 0.5f, 0.5f);
        C2D_SpriteSetPos(&text, SCREEN_WIDTH_AREA / 2, SCREEN_HEIGHT_AREA / 2);
        C2D_SpriteSetScale(&text, scale, scale);
        C2D_DrawSprite(&text);
    }
}

}
