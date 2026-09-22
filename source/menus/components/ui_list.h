
#include "menus/core/ui_screen.h"

void ui_list_add(UIList* list, UIElement* item);
void ui_list_reset(UIList *list);
void ui_list_set_capacity(UIList *list, size_t capacity);

UIList *ui_create_list(UIScreen *screen);
UIElement *ui_create_list_from_props(UIScreen *screen, const UIPropertyList *props);