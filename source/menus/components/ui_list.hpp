
#include "menus/core/ui_screen.hpp"

void uiListAdd(UIList* list, UIElement* item);
void uiListReset(UIList *list);
void uiListSetCapacity(UIList *list, size_t capacity);

UIList *uiCreateList(UIScreen *screen);
UIElement *uiCreateListFromProps(UIScreen *screen, const UIPropertyList *props);