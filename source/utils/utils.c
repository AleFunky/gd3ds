#include "utils.h"

u8 getModel() {
    u8 model;
    CFGU_GetSystemModel(&model);
    return model;
}
