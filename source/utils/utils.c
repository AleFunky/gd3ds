#include "utils.h"

u8 get_model() {
    u8 model;
    CFGU_GetSystemModel(&model);
    return model;
}
