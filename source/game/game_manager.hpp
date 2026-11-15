#pragma once

#include <memory>

namespace gd3ds
{

struct PlatformState {
    bool isN3Ds = false;
    bool is3Dsx = false;
    bool isGDPS = false;
    bool isNightly = false;
    bool inExclusiveState = false;
    bool has3DWarningPassed = false;
    const char* _3dsxPath = "";
};

class GameManager {
public:
    static GameManager* get() {
        if (!s_instance) {
            s_instance = std::make_unique<GameManager>();
        }
        return s_instance.get();
    }

// public fields
public:
    PlatformState platformState{};
    bool restartQueued = false;
    bool exitingLevel  = false;
    bool songLoaded    = false;
    bool cheated = false;
    bool inLevelComplete = false;
    bool gamePaused = false;
    
    float dimSecondaryScreen = 0.0f;

    // frame stuff
    float deltaTime = 0;
    float frameTimer = 0;
    unsigned int levelFrame = 0;
    unsigned int frameCounter = 0;
    float accumulator = 0.f;
    bool fixedDt = true;
    int steps;
    int lastSteps;

    bool altTitleScreen = false;

private:
    static std::unique_ptr<GameManager> s_instance;
};

}