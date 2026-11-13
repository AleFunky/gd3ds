#pragma once

#include "utils/json_config.hpp"

#define DEFAULT_P1 39
#define DEFAULT_P2 51
#define DEFAULT_GLOW 51

namespace gd3ds
{

extern Config cfg;

void cfgInit();
void cfgSave();
void cfgFini();

}