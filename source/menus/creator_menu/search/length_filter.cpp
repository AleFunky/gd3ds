#include <3ds.h>
#include <citro2d.h>

#include "main.hpp"
#include "menus/core/ui_screen.hpp"
#include "utils/server_utils.hpp"

static void update_length_tint(UIElement *e){
    UILabel *l = (UILabel *)e;
    int opacity = (filters.lengthFilters & (ui_prop_int(&e->custom_properties, "lengthval", 0))) > 0 ? 255 : 127;

    int length_bit = ui_prop_int(&e->custom_properties, "lengthval", 0);
    int length = __builtin_ctz(length_bit);

    const char *length_str = "Unkn.";
    if (IN_BOUNDS((size_t)length, level_lengths)) {
        length_str = (const char *) level_lengths[length];
    }

    snprintf(l->text, sizeof(l->text), "<%d,%d,%d>%s</>", opacity, opacity, opacity, length_str);
}

static void update_length_tints(UIScreen *s){
    ui_run_func_on_tag(s, "lengthbtn", update_length_tint);
}

static void action_set_length(UIElement* e, const UIPropertyList *props) {
    filters.lengthFilters ^= ui_prop_int(&e->custom_properties, "lengthval", 0);
    update_length_tints(e->screen);
}

static UIActionDef length_filter_actions[] = {
    { "length", action_set_length },
};

void length_filter_init(UIScreen *s) {
    update_length_tints(s);
}

extern const UIScreenDefPair length_filter_def = {
    .name = "length_filter",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/length_filter_pop_up.txt",
        .init = length_filter_init,
        .action_list = {
            .actions = length_filter_actions,
            .action_count = ARRAY_LEN(length_filter_actions)
        }
    }
};