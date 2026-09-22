#include "ui_element.h"
#include "ui_screen.h"

void ui_element_set_userdata(UIElement *element, void *userdata) {
    element->userdata = userdata;
}

bool ui_element_basic_bound_check(UIElement *e, UIInput *touch, UITransform *transform) {
    if (touch->touchPosition.px == 0 && touch->touchPosition.py == 0) return false;

    float width  = (e->w * 0.5f) * fabsf(transform->scaleX);
    float height = (e->h * 0.5f) * fabsf(transform->scaleY);

    return touch->touchPosition.px >= transform->x - width  && touch->touchPosition.px < transform->x + width &&
           touch->touchPosition.py >= transform->y - height && touch->touchPosition.py < transform->y + height;
}

// Combines a transform an element's data
UITransform ui_transform_combine(UITransform *parent, UIElement *e) {
    return (UITransform){
        .x = parent->x + e->x * parent->scaleX,
        .y = parent->y + e->y * parent->scaleY,

        .scaleX = parent->scaleX * e->scaleX,
        .scaleY = parent->scaleY * e->scaleY,
    };
}

// Updates an element and its children
void ui_update_tree(UIElement *e, UIInput *input, UITransform *parent) {
    if (!e->enabled) return;

    UITransform world = ui_transform_combine(parent, e);
    
    // Let the element modify its own transform
    if (e->modify_transform)
        e->modify_transform(e, &world);
    
    if(e->update){
        e->update(e, input, &world);
    }

    if (!e->draws_children) {
        for (UIElement *child = e->first_child; child; child = child->next_sibling) {
            ui_update_tree(child, input, &world);
        }
    }
}

// Draws an element and its children
void ui_draw_tree(UIElement *e, UITransform *parent) {
    if (!e->enabled) return;

    UITransform world = ui_transform_combine(parent, e);

    // Let the element modify its own transform
    if (e->modify_transform)
        e->modify_transform(e, &world);

    e->draw(e, &world);

    if (!e->draws_children) {
        for (UIElement *child = e->first_child; child; child = child->next_sibling) {
            ui_draw_tree(child, &world);
        }
    }
}

// Destroys an element and its children
void ui_destroy_tree(UIElement *e) {
    UIElement *child = e->first_child;

    while (child) {
        UIElement *next = child->next_sibling;

        ui_destroy_tree(child);

        child = next;
    }

    if (e->userdata && e->userdata_destroy) e->userdata_destroy(e->userdata);

    if(e->actions){
        for(int i = 0; i < e->action_count; i++){
            ui_destroy_proplist(&e->actions[i].args);
        }
        free(e->actions);
    }

    ui_destroy_proplist(&e->custom_properties);
    
    e->destroy(e);
}

void ui_enable_element(UIElement *e) { 
    e->enabled = true;

    if (e->on_enable) {
        e->on_enable(e);
    }
}

void ui_disable_element(UIElement *e) { 
    e->enabled = false;
    
    if (e->on_disable) {
        e->on_disable(e);
    }
}

void ui_element_add_child(UIElement *parent, UIElement *child) {
    if (!parent || !child) return;

    child->parent = parent;

    child->next_sibling = NULL;
    child->prev_sibling = parent->last_child;

    if (parent->last_child)
        parent->last_child->next_sibling = child;
    else
        parent->first_child = child;

    parent->last_child = child;
}

void ui_element_remove(UIElement *element) {
    if (!element->parent) return;

    UIElement *parent = element->parent;

    if (element->prev_sibling)
        element->prev_sibling->next_sibling = element->next_sibling;
    else
        parent->first_child = element->next_sibling;

    if (element->next_sibling)
        element->next_sibling->prev_sibling = element->prev_sibling;
    else
        parent->last_child = element->prev_sibling;

    element->parent = NULL;
    element->next_sibling = NULL;
    element->prev_sibling = NULL;
}

UIElement *ui_get_child_by_type(UIElement *parent, UIElementType type) {
    for (UIElement *child = parent->first_child; child; child = child->next_sibling) {
        if (child->type == type) {
            return child;
        }
    }

    return NULL;
}

UIActionFn ui_find_action(const UIActionDef* actions, size_t count, const char* name) {
    for (size_t i = 0; i < count; i++)
        if (strcmp(actions[i].name, name) == 0)
            return actions[i].fn;
    return NULL;
}

void ui_element_apply_properties(UIElement *e, UIScreen *screen, const UIPropertyList *props) {
    if (!e || !screen || !props) return;

    ui_element_set_position(e, 
        ui_prop_int(props, "x", e->x), 
        ui_prop_int(props, "y", e->y));

    ui_element_set_size(e, 
        ui_prop_int(props, "w", e->w), 
        ui_prop_int(props, "h", e->h));
    
    float scale = ui_prop_float(props, "scale", 1.0f);
    ui_element_set_scale_xy(e,
        ui_prop_float(props, "sx", scale),
        ui_prop_float(props, "sy", scale)
    );

    // Copy tag
    copy_tag_array(e, ui_prop_string(props, "tag", ""));
    
    e->opacity = ui_prop_float(props, "opacity", 1);

    const UIActionDef* actions = (screen->def && screen->def->action_list.actions) ? screen->def->action_list.actions : NULL;
    const size_t action_count = (screen->def && screen->def->action_list.actions) ? screen->def->action_list.action_count : 0;

    e->action = ui_find_action(
        actions, action_count,
        ui_prop_string(props, "action", "")
    );

    e->actions = ui_prop_actions(
        props, 
        actions, action_count,
        "actions",
        &e->action_count
    );

    e->custom_properties = ui_prop_list(props, "custom");
}

void ui_element_apply_default_properties(UIElement *e, UIScreen *screen) {
    if (!e || !screen) return;

    ui_element_set_scale(e, 1);

    e->opacity = 1;

    e->screen = screen;

    e->userdata_destroy = free;
}

void perform_actions(UIElement *e){
    if (e->action){
        e->action(e, NULL);
    }

    if(e->actions){
        for(int i = 0; i < e->action_count; i++){
            if(e->actions[i].action){
                e->actions[i].action(e, &e->actions[i].args);
            }
        }
    }
}