#include <3ds.h>
#include <citro2d.h>

#include "menus/core/ui_screen.h"
#include "menus/components/ui_label.h"
#include "menus/components/ui_checkbox.h"
#include "menus/components/ui_window_button.h"
#include "search_filters.h"
#include "utils/server_utils.h"

static void update_length_tint(UIElement *e){
    UILabel *l = (UILabel *)e;
    int opacity = (filters.lengthFilters & (ui_prop_int(&e->custom_properties, "lengthval", 0))) > 0 ? 255 : 127;

    int length_bit = ui_prop_int(&e->custom_properties, "lengthval", 0);
    int length = __builtin_ctz(length_bit);

    char *length_str = "Unkn.";
    if (IN_BOUNDS(length, level_lengths)) {
        length_str = (char *) level_lengths[length];
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

const UIScreenDefPair length_filter_def = {
    .name = "length_filter",
    .btm = {
        .path = "romfs:/menus/creator_menu/search/length_filter_pop_up.txt",
        .init = length_filter_init,
        .action_list = {
            .action_count = ARRAY_LEN(length_filter_actions),
            .actions = length_filter_actions
        }
    }
};