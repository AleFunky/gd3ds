#pragma once

#include "utils/json_config.hpp"

#define DEFAULT_P1 39
#define DEFAULT_P2 51
#define DEFAULT_GLOW 51

extern Config cfg;

void cfg_init();
void cfg_save();
void cfg_fini();