#include <citro2d.h>
#include <citro3d.h>

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <3ds.h>
#include <archive.h>
#include "main.hpp"

#include <curl/curl.h>

#include "particles/particles.hpp"

#include "game/game.hpp"

#ifdef DEBUG_LEAKS
#include "utils/leaks_dbg.hpp"
#endif

namespace gd3ds
{

Arena levelArena;

u32 ctruLinearHeapSize __asm__("__ctru_linear_heap_size") = 34 << 20;

int gameState = static_cast<int>(GameState::Menu);
bool escapeState;

bool playingMenuLoop = false;
char menuLoopPath[32];

int levelResult = 0;

bool cheatsUsed[static_cast<int>(Cheats::CheatCount)];

const char *cheatNames[static_cast<int>(Cheats::CheatCount)] = {
    "Noclip",
    "Hitboxes"
};

PrintConsole console;

C3D_RenderTarget* top;
C3D_RenderTarget* topRight;
C3D_RenderTarget* bot;

ParticleSystem touchDragParticles;
ParticleSystem touchExplosionParticles;
ParticleSystem glitterParticlesBottom;
ParticleSystem slowSpeedParticlesBottom;
ParticleSystem normalSpeedParticlesBottom;
ParticleSystem fastSpeedParticlesBottom;
ParticleSystem fasterSpeedParticlesBottom;
ParticleSystem fastestSpeedParticlesBottom;
ParticleSystem endWallParticles;
ParticleSystem endWallFirework;
ParticleSystem levelCompleteEffectP1;
ParticleSystem levelCompleteEffectP2;

float slowSpeedParticlesTimer = 0.f;
float normalSpeedParticlesTimer = 0.f;
float fastSpeedParticlesTimer = 0.f;
float fasterSpeedParticlesTimer = 0.f;
float fastestSpeedParticlesTimer = 0.f;

}

int main(int argc, char* argv[]) {
    using namespace gd3ds;
    
    game::init(argc, argv);
    
    while (aptMainLoop()) {
        if (!game::update()) break;
    }
    
    game::close();

    return 0;
}

