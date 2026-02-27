#include "keyboard.h"

SwkbdState swkbd;

SwkbdButton read_text(char *text, char *title, int limit) {
    swkbdInit(&swkbd, SWKBD_TYPE_QWERTY, 1, limit);
    swkbdSetHintText(&swkbd, title);
    swkbdSetInitialText(&swkbd, text);
    swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN);
    return swkbdInputText(&swkbd, text, limit + 1);
}