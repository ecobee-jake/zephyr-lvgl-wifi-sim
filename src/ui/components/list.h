/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_UI_COMPONENTS_LIST_H_
#define APP_UI_COMPONENTS_LIST_H_

#include <lvgl.h>
#include <stddef.h>

/* Always single-select: checking one item unchecks the others. */
lv_obj_t *ui_list_create(lv_obj_t *parent, const char **items, size_t count);

/* Replaces the list's contents. Shows a "No networks found" message when count is 0. */
void ui_list_set_items(lv_obj_t *list, const char **items, size_t count);

/* Called with LV_EVENT_CLICKED when any item is pressed, in addition to the
 * built-in single-select handling. Applies to items added afterwards too.
 */
void ui_list_set_item_click_cb(lv_obj_t *list, lv_event_cb_t cb);

#endif /* APP_UI_COMPONENTS_LIST_H_ */
