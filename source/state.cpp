#include "state.hpp"
#include <string.h>
#include "player/player.hpp"
#include "player/robot_anim_data.hpp"
#include "main.hpp"
#include "math_helpers.hpp"
#include "player/slope.hpp"
#include "mp3_player.hpp"
#include "player/collision.hpp"
#include "particles/circles.hpp"
#include "particles/particles.hpp"
#include "menus/settings_hub/settings.hpp"
#include "menus/icon_kit.hpp"
#include "practice.hpp"
#include "sounds.hpp"
#include "utils/json_config.hpp"
#include "level/main_levels.hpp"
#include "level_loading.hpp"
#include "menus/level_select.hpp"
#include "save/saving.hpp"

#include "new_best.hpp"
#include "endwall.hpp"
#include "wav_player.hpp"

#define FIRST_ATTEMPT_CAMERA_OFFSET 15
#define PLAYER_CAMERA_X_OFFSET 125.0f
#define PLAYER_CAMERA_X_OFFSET_BOT (PLAYER_CAMERA_X_OFFSET * (SCREEN_BOT_WIDTH / (float)SCREEN_WIDTH))

GamePlayState state;

bool levelUnratedOnline = false;
bool levelVerifiedCoins = false;

bool levelHasUnverifiedCoins(void) {
    if (state.customLevel && !state.onlineLevel) return false; // External level
    if (!state.customLevel) return false; // Main level
    return levelUnratedOnline || !levelVerifiedCoins;
}

void levelUnratedOnlineRefresh(void) {
    levelUnratedOnline = state.onlineLevel
        && currentLevelEntry
        && !levelHasRate(&currentLevelEntry->data);
}


u32 userCoinIconTint(bool collected, bool hasRate) {
    if (collected) return hasRate ? C2D_Color32(255, 255, 255, 255) : USER_COIN_UNRATED_TINT;
    return hasRate ? C2D_Color32(165, 165, 165, 255) : USER_COIN_UNRATED_EMPTY_TINT;
}

void setCameraX(float x) {
    state.cameraX = x;
    state.unmodCamX = x;
}

void setCameraY(float y) {
    state.cameraY = y;
    state.unmodCamY = y;
}

void runCamera() {
    Player *player = &state.player;
    float playerCameraXOffset = settingsState.swapGameplayScreens ? PLAYER_CAMERA_X_OFFSET_BOT : PLAYER_CAMERA_X_OFFSET;
    state.oldCameraX = state.cameraX;
    state.oldCameraY = state.cameraY;

    float playableHeight = state.ceilingY - state.groundY;
    float calcHeight = 0;

    if ((player->gamemode != GAMEMODE_PLAYER && player->gamemode != GAMEMODE_ROBOT) || state.dual) {
        calcHeight = (SCREEN_HEIGHT_AREA - playableHeight) / 2;
    }
    state.groundYGfx = easeOut(state.groundYGfx, calcHeight, 0.02f);

    if (levelInfo.wallY == 0) {
        if (state.cameraX + SCREEN_WIDTH_AREA >= levelInfo.wallX - (4.5f * 30.f)) {
            levelInfo.wallY = MAX(state.cameraYMiddle, 60 + ((SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET));
        }
    }

    float camera_x_right = state.cameraX + SCREEN_WIDTH_AREA;

    // Cap at camera_x
    if (levelInfo.wallY > 0 && (camera_x_right >= levelInfo.wallX - CAMERA_X_WALL_OFFSET)) {
        if (state.cameraWallTimer == 0) {
            state.backgroundWallInitialX = state.backgroundX;
            state.groundWallInitialX = state.groundX;
        }
        state.backgroundX = easeValue(EASE_IN_OUT, state.backgroundWallInitialX, state.backgroundWallInitialX + CAMERA_X_WALL_OFFSET * state.mirrorSpeedFactor, state.cameraWallTimer, 1.f, 2.0f);            
        state.groundX = easeValue(EASE_IN_OUT, state.groundWallInitialX, state.groundWallInitialX + CAMERA_X_WALL_OFFSET * state.mirrorSpeedFactor, state.cameraWallTimer, 1.f, 2.0f);            
    }

    if (levelInfo.wallY > 0 && (camera_x_right >= levelInfo.wallX - CAMERA_X_WALL_OFFSET)) {
        if (state.cameraWallTimer == 0) {
            state.cameraWallInitialY = state.cameraY;
        }

        float finalCameraXWall = levelInfo.wallX - (SCREEN_WIDTH_AREA);
        float finalCameraYWall = levelInfo.wallY - ((SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET);   

        setCameraX(easeValue(EASE_IN_OUT, finalCameraXWall - CAMERA_X_WALL_OFFSET, finalCameraXWall, state.cameraWallTimer, CAMERA_WALL_ANIM_DURATION, 2.0f));
        setCameraY(easeValue(EASE_IN_OUT, state.cameraWallInitialY, finalCameraYWall, state.cameraWallTimer, CAMERA_WALL_ANIM_DURATION, 2.0f));
        state.cameraWallTimer += STEPS_DT;
    } else { 
        float camY = state.cameraY;

        float targetY = camY;

        if ((player->gamemode == GAMEMODE_PLAYER || player->gamemode == GAMEMODE_ROBOT) && !state.dual) {
            float playerY = player->y;

            float upsideDownOffset = (player->upsideDown ? -30.f : 0);

            if (playerY > camY + 140.f + upsideDownOffset) { // Top margin
                targetY = playerY - (140.f + upsideDownOffset);
            } else if (playerY < camY + 30.f + upsideDownOffset) { // Bottom margin
                targetY = playerY - (30.f + upsideDownOffset);
            }
        } else {
            targetY = state.cameraIntendedY;
        }
            
        if (targetY < 0) {
            targetY = 0;
        }
        
        camY += (targetY - camY) / (10 / 0.25f);
        
        if (camY < 0) {
            camY = 0;
        }
        
        if (camY > MAX_LEVEL_HEIGHT - SCREEN_HEIGHT_AREA) {
            camY = MAX_LEVEL_HEIGHT - SCREEN_HEIGHT_AREA;
        }

        setCameraY(camY);

        setCameraX(player->x - playerCameraXOffset/SCALE);
        
        if (state.currentData.attempts == 1) {
            if (state.cameraX < FIRST_ATTEMPT_CAMERA_OFFSET) {
                setCameraX(FIRST_ATTEMPT_CAMERA_OFFSET);
            }
        }

        if (state.currentData.attempts > 1 || player->x - playerCameraXOffset/SCALE >= FIRST_ATTEMPT_CAMERA_OFFSET) {
            state.groundX += player->velX * STEPS_DT * state.mirrorSpeedFactor;
            state.backgroundX += player->velX * STEPS_DT * state.mirrorSpeedFactor;
        }
    }
    
    state.cameraXMiddle = state.cameraX + (SCREEN_WIDTH_AREA / 2);
    state.cameraYMiddle = state.cameraY + (SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET;
}

void setHitboxSize(Player *player, int gamemode) {
    float scale = (player->mini) ? 0.6f : 1.f;
    if (gamemode == GAMEMODE_SPIDER) {
        player->height = 27 * scale;
        player->width = 27 * scale;

        player->internalHitbox.width = 9;
        player->internalHitbox.height = 9;
    } else if (gamemode != GAMEMODE_WAVE) {
        player->height = 30 * scale;
        player->width = 30 * scale;
        
        player->internalHitbox.width = 9;
        player->internalHitbox.height = 9;
    } else {
        player->height = 10 * scale;
        player->width = 10 * scale;

        player->internalHitbox.width = 3;
        player->internalHitbox.height = 3;
    }
}

void setIntendedCeiling() {
    float midPoint = (state.groundY + state.ceilingY) / 2;
    state.cameraIntendedY = midPoint - ((SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET);
}

void setGamemode(Player *player, int gamemode) {
    if (player->gamemode != gamemode) player->platformVelocity = player->lastPlatformVelocity = 0.f;
    player->ballSlopeRotationMult = 1.f;
    player->gamemode = gamemode;
    setHitboxSize(player, gamemode);
}

void setMini(Player *player, bool mini) {
    player->mini = mini;
    setHitboxSize(player, player->gamemode);
}

void updateAttemptTextPos() {
    // Set attempt text positions
    if (state.currentData.attempts == 1) {
        state.attemptTextPos.x = SCREEN_WIDTH_AREA / 2;
    } else {
        state.attemptTextPos.x = state.player.x + (5 * 30);
    }

    state.attemptTextPos.y = state.cameraY + (5 * 30);
}

void clearSnapData(Player *player) {
    player->snapData.snappedObj = -1;
    player->snapData.playerFrame = 0;
    player->snapData.playerSnapDiff = 0;
    player->snapData.objectID = -1;
}

void initPlayer(Player *player) {
    memset(player, 0, sizeof(Player));
    
    setGamemode(player, levelInfo.initialGamemode);
    setMini(player, levelInfo.initialMini);

    setBlendTimer(player, getBlendDuration(player));

    player->playerIcons.cube = selectedCube;
    player->playerIcons.ship = selectedShip;
    player->playerIcons.ball = selectedBall;
    player->playerIcons.ufo  = selectedUfo;
    player->playerIcons.wave = selectedWave;
    player->playerIcons.robot = selectedRobot;
    player->playerIcons.spider = selectedSpider;
    player->playerIcons.glow = playerGlowEnabled;

    player->playerIcons.p1Color = p1Color;
    player->playerIcons.p2Color = p2Color;
    player->playerIcons.glowColor = glowColor;
    
    player->cutsceneTimer = 0;
    player->x = 0;
    player->y = 15;
    player->velX = playerSpeeds[state.speed];  
    player->velY = 0;
    player->newVelY = __FLT_MAX__;
    player->ballSlopeRotationMult = 1.f;
    player->platformVelocity = player->lastPlatformVelocity = 0.f;
    player->frame = 0;

    player->onGround = true;
    player->onCeiling = false;
    player->inverseRotation = false;
    player->upsideDown = levelInfo.initialUpsidedown;
    player->timeElapsed = 0.f;

    player->cutsceneInitialPlayerX = 0;
    player->cutsceneInitialPlayerY = 0;

    player->allowDartSlide = false;
    player->allowHeadHit = false;
    player->lastOrbPressed = -1;
    clearSlopeData(player);
    clearSnapData(player);
}

void clearP1Trail() {
    memset(state.p1TrailData, 0, sizeof(state.p1TrailData));
    state.p1TrailPos[0] = 0;
    state.p1TrailPos[1] = 0;
    p1TrailTimer = 0;
}

void initState() {
    state.currentPlayer = 0;

    state.cameraX = 0;

    state.cameraWallTimer = 0;
    state.cameraWallInitialY = 0;

    state.mirroring = false;
    state.originalMirrorFactor = 0.f;
    state.intendedMirrorFactor = 0.f;
    state.mirrorFactor = 0.f;
    state.mirrorSpeedFactor = 1.f;
    state.mirrorMult = 1;

    state.hitboxEnabledWhenDead = false;
    state.deathTimer = 0.f;

    state.dual = false;
    state.dead = false;
    state.mirrorMult = 1;
    state.speed = levelInfo.initialSpeed;
    state.groundY = 0;
    state.ceilingY = 999999;
    
    levelInfo.wallY = 0;

    state.endWallAnimPlaying = false;
    
    clearP1Trail();

    state.currentData.attempts++;
    state.currentData.coin1 = false;
    state.currentData.coin2 = false;
    state.currentData.coin3 = false;
    state.lastHitboxTrail = 0;

    state.showPlayer = true;

    inLevelComplete = false;

    cheated = false;
    for (int i = 0; i < CHEAT_COUNT; i++) {
        cheatsUsed[i] = false;
    }
}

void initLevelBounds() {
    switch (levelInfo.initialGamemode) {
        default:
            state.ceilingY = state.groundY + gamemodeHeights[levelInfo.initialGamemode];
            setIntendedCeiling();
            break;
        case GAMEMODE_PLAYER:
        case GAMEMODE_ROBOT:
            state.cameraIntendedY = 0;
    }
    
    if (levelInfo.initialDual) {
        state.dual = true;
        state.dualPortalY = 0.f;
        setupDual();
        setDualBounds();
    }
    runCamera();

    // Set camera vertical pos
    state.cameraY = state.cameraIntendedY;
    state.cameraYMiddle = state.cameraY + ((SCREEN_HEIGHT_AREA / 2) - LEVEL_Y_OFFSET);

    float playableHeight = state.ceilingY - state.groundY;
    float calcHeight = 0;

    if ((state.player.gamemode != GAMEMODE_PLAYER && state.player.gamemode != GAMEMODE_ROBOT) || state.dual) {
        calcHeight = (SCREEN_HEIGHT_AREA - playableHeight) / 2;
    }
    
    state.groundYGfx = calcHeight; 

    updateAttemptTextPos();
}

void firstLoadInitVariables() {
    levelUnratedOnlineRefresh();
    memset(&state.currentData, 0, sizeof(StateLevelData));
    
    levelInfo.wallX = roundf(levelInfo.lastObjX / 30) * 30;
    levelInfo.wallY = 0;

    state.backgroundX = 0;
    state.groundX = 0;
    
    state.cameraX = 0;
    state.cameraY = 0;
    currentFadingEffect = FADE_SIMPLE;

    slowSpeedParticlesTimer = 0;
    normalSpeedParticlesTimer = 0;
    fastSpeedParticlesTimer = 0;
    fasterSpeedParticlesTimer = 0;
    fastestSpeedParticlesTimer = 0;

    clearNewBestPopup();
    initVariables();
    
    // Run triggers behind the start
    handleTriggers();
    
    LevelData *levelDataSel = &currentLevelEntry->data;
    state.currentData.timeStart = svcGetSystemTick() / (CPU_TICKS_PER_MSEC * 1000);
    state.currentData.maxNormal = levelDataSel->normalProgress;
    state.currentData.maxPractice = levelDataSel->practiceProgress;
}

void initWaveTrails() {
    C2D_Image img = C2D_SpriteSheetGetImage(trailSheet, 0);
    Color usedP1 = (settingsState.switchWaveTrailColor ? p1Color : p2Color);
    Color usedP2 = (settingsState.switchWaveTrailColor ? p2Color : p1Color);
    
    motionTrailInit(&waveTrailP1, 0, 3.f, true, 10.0f, true, (usedP1.r | usedP1.g | usedP1.b && !settingsState.solidWaveTrail), false, usedP1, img);
    motionTrailInit(&waveTrailP2, 1, 3.f, true, 10.0f, true, (usedP2.r | usedP2.g | usedP2.b && !settingsState.solidWaveTrail), false, usedP2, img);
}

void initTrails(int trail) {
    const MotionTrailConfig *config = &trailProperties[trail];

    C2D_Image img = C2D_SpriteSheetGetImage(trailSheet, trail);

    Color usedP1 = (settingsState.switchTrailColor ? p1Color : p2Color);
    Color usedP2 = (settingsState.switchTrailColor ? p2Color : p1Color);

    if (!config->colored) {
        usedP1 = white;
        usedP2 = white;
    }

    C3D_TexSetFilter(img.tex, GPU_LINEAR, GPU_LINEAR);
    motionTrailInit(&trailP1, 0, config->fade, config->alwaysOn, config->width, false, config->blending, config->stationary, usedP1, img);
    motionTrailInit(&trailP2, 1, config->fade, config->alwaysOn, config->width, false, config->blending, config->stationary, usedP2, img);

    motionTrailStopStroke(&trailP1);
    motionTrailStopStroke(&trailP2);
}

void initVariables() {
    levelFrame = 0;
   
    initTrails(selectedTrail);
    initWaveTrails();
    initShake();

    clearUseEffects(getUseEffectArrayPtr(GFX_TOP));

    currentFadingEffect = FADE_SIMPLE;
    levelInfo.completing = false;

    initPlayer(&state.player);
    initPlayer(&state.player2);

    initState();

    initLevelBounds();

    clearLevelCompletePopup();

    p1Trail = false;

    clearBgFlash();
    if (gameState == STATE_GAME && state.currentData.attempts != 1) startRespawnEffect();
}

void handleDeath(Player *player, bool pauseSong) {
    state.deathPlayer = state.player;
    playSfx(&explodeSound, 1);
    if (songLoaded && pauseSong) {
        if (!state.practiceMode) {
            pausePlaybackMP3();
            seekMP3(levelInfo.songOffset);
        } else if (settingsState.practiceMusicSync) {
            pausePlaybackMP3();
        }
    }

    if (state.showPlayer) {
        // Spawn death particles
        UseEffect *effect = addUseEffect(player->x, player->y, USE_EFFECT_OBJ_NOTHING, &deathEffect, getUseEffectArrayPtr(GFX_TOP));
        if (effect) {
            Color colorNotWhite = getWhiteIfBlack((state.currentPlayer == 1 ? p2Color : p1Color));

            effect->def.colorR = colorNotWhite.r / 255.f;
            effect->def.colorG = colorNotWhite.g / 255.f;
            effect->def.colorB = colorNotWhite.b / 255.f;

            effect->def.endRad *= (player->mini ? 0.6 : 1.0f);
            effect->def.startRad *= (player->mini ? 0.6 : 1.0f);
        }

        explosionParticles[state.currentPlayer].emitterX = player->x;
        explosionParticles[state.currentPlayer].emitterY = player->y;
        explosionParticles[state.currentPlayer].scale = (player->mini ? 0.6 : 1.0f);
        spawnMultipleParticles(&explosionParticles[state.currentPlayer], 90);
    }

    if (!state.practiceMode) startShake(0.15f, 1.f);
    
    if (settingsState.hitboxesOnDeath) {
        state.hitboxEnabledWhenDead = true;
    }
}

// Size portal flashing

void startBgFlash() {
    BGFlashData *data = &state.flashData;
    data->flashing = true;
    data->timer = FLASH_TIME_1;
    data->state = FLASH_FIRST_LIGHT;
    data->useLbg = true;
}

void handleBgFlash() {
    BGFlashData *data = &state.flashData;
    if (!data->flashing) return;

    // Run flashing state
    switch (data->state) {
        case FLASH_NONE:
            break;
        
        case FLASH_FIRST_LIGHT:
            data->timer -= STEPS_DT;
            if (data->timer <= 0) {
                data->state = FLASH_UNLIGHTED;
                data->timer = FLASH_TIME_2;
                data->useLbg = false;
            }
            break;
        case FLASH_UNLIGHTED:
            data->timer -= STEPS_DT;
            if (data->timer <= 0) {
                data->state = FLASH_SECOND_LIGHT;
                data->timer = FLASH_TIME_2;
                data->useLbg = true;
            }
            break;
        case FLASH_SECOND_LIGHT:
            data->timer -= STEPS_DT;
            if (data->timer <= 0) {
                // End flash
                data->state = FLASH_NONE;
                data->timer = 0;
                data->useLbg = false;
                data->flashing = false;
            }
            break;
    }
}

void clearBgFlash() {
    state.flashData.flashing = false;
    state.flashData.useLbg = false;
}

void playLevelSong(float seek) {
    if (levelInfo.customSongID > 0 && state.customLevel) {
        size_t sz = 0;
        void *buf = loadUserSong(levelInfo.customSongID, &sz);
        if (buf) {
            songLoaded = playMP3Buf(buf, sz, false, seek, false);
        } else {
            songLoaded = false;
        }
    } else {
        size_t songID = currLevelID;

        if (state.customLevel) songID = levelInfo.songID;

        if (songID < currentMainLevelPack->count) {
            songLoaded = playMP3(currentMainLevelPack->levels[songID].songPath, false, seek);
        } else {
            songLoaded = false;
        }
    }
}

void playPracticeSong() {
    playMP3("romfs:/songs/StayInsideMe.mp3", true, 0);
}

void playMenuSong() {
    if (!settingsState.menuMusic) return;

    if (!playingMenuLoop) {
        size_t outSize;
        void *buf = readFileArena(&levelArena, menuLoopPath, &outSize);
        if (buf) {
            playMP3Buf(buf, outSize, true, 0, false);
            playingMenuLoop = true;
        }
    }
}

// Respawn effect

void startRespawnEffect() {
    RespawnEffectData *data = &state.respawnEffectData;
    data->active = true;
    data->timer = RESPAWN_EFFECT_DURATION;
    data->state = RESPAWN_EFFECT_HIDE_PLAYER;
    data->remaining = RESPAWN_EFFECT_REPEAT;
    data->hidePlayer = true;
}

void handleRespawnEffect() {
    RespawnEffectData *data = &state.respawnEffectData;
    if (!data->active) return;

    // Run respawn state
    switch (data->state) {
        case RESPAWN_EFFECT_NONE:
            break;
        
        case RESPAWN_EFFECT_HIDE_PLAYER:
            // First frame
            if (data->timer == RESPAWN_EFFECT_DURATION) {
                UseEffect *effectP1 = addUseEffect(state.player.x, state.player.y, USE_EFFECT_OBJ_P1, &respawnEffect, getUseEffectArrayPtr(GFX_TOP));
                if (effectP1) {
                    Color colorNotWhite = getWhiteIfBlack(p1Color);

                    effectP1->def.colorR = colorNotWhite.r / 255.f;
                    effectP1->def.colorG = colorNotWhite.g / 255.f;
                    effectP1->def.colorB = colorNotWhite.b / 255.f;
                }

                if (state.dual) {
                    UseEffect *effectP2 = addUseEffect(state.player2.x, state.player2.y, USE_EFFECT_OBJ_P2, &respawnEffect, getUseEffectArrayPtr(GFX_TOP));
                    if (effectP2) {
                        Color colorNotWhite = getWhiteIfBlack(p2Color);

                        effectP2->def.colorR = colorNotWhite.r / 255.f;
                        effectP2->def.colorG = colorNotWhite.g / 255.f;
                        effectP2->def.colorB = colorNotWhite.b / 255.f;
                    }
                }
            }

            data->timer -= STEPS_DT;
            if (data->timer <= 0) {
                data->state = RESPAWN_EFFECT_SHOW_PLAYER;
                data->timer = RESPAWN_EFFECT_DURATION;
                data->hidePlayer = false;
            }
            break;
        case RESPAWN_EFFECT_SHOW_PLAYER:
            data->timer -= STEPS_DT;
            if (data->timer <= 0) {
                data->hidePlayer = true;
                if (--data->remaining == 0) {
                    data->timer = 0;
                    data->state = RESPAWN_EFFECT_NONE;
                    data->active = false;
                    break;
                }
                
                data->timer = RESPAWN_EFFECT_DURATION;
                data->state = RESPAWN_EFFECT_HIDE_PLAYER;
            }
            break;
    }
}

void clearRespawnEffect() {
    state.respawnEffectData.active = false;
    state.respawnEffectData.hidePlayer = false;
}

bool isCoinCollected(int obj) {
    if (!currentLevelEntry) return false;

    if (objects.id[obj] != SECRET_COIN) return false;

    LevelData *levelData = &currentLevelEntry->data;

    switch (objects.coinID[obj]) {
        case 0:
            if (levelData->coin1) 
                return true;
            break;
        case 1:
            if (levelData->coin2) 
                return true;
            break;
        case 2:
            if (levelData->coin3) 
                return true;
            break;
    }

    return false;
}

void killPlayer(DeathReason reason) {
    state.dead = true;
    state.deathReason = reason;
}

void initShake() {
    state.shakeData.active = false;
}

void startShake(float duration, float strength) {
    state.shakeData.timer = duration;
    state.shakeData.strength = strength;
    state.shakeData.active = true;
}

void handleShake(float delta) {
    if (state.shakeData.active) {
        state.cameraX = state.unmodCamX + state.shakeData.strength * randomFloat(-1, 1);
        state.cameraY = state.unmodCamY + state.shakeData.strength * randomFloat(-1, 1);
        state.shakeData.timer -= delta;

        // End of shake
        if (state.shakeData.timer < 0) {
            state.shakeData.active = false;
            state.cameraX = state.unmodCamX;
            state.cameraY = state.unmodCamY;
        }
    }
}