/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "list.h"

static void select_event_cb(lv_event_t *e)
{
	lv_obj_t *clicked = (lv_obj_t *)lv_event_get_target(e);
	lv_obj_t *list = lv_obj_get_parent(clicked);
	uint32_t count = lv_obj_get_child_count(list);

	for (uint32_t i = 0; i < count; i++) {
		lv_obj_t *child = lv_obj_get_child(list, i);

		if (child != clicked) {
			lv_obj_remove_state(child, LV_STATE_CHECKED);
		}
	}
}

static void add_item(lv_obj_t *list, const char *text)
{
	lv_obj_t *btn = lv_list_add_button(list, NULL, text);
	/* Item click callback is kept on the list itself so it survives the
	 * lv_obj_clean() in ui_list_set_items().
	 */
	lv_event_cb_t item_cb = (lv_event_cb_t)lv_obj_get_user_data(list);

	lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
	lv_obj_add_event_cb(btn, select_event_cb, LV_EVENT_CLICKED, NULL);

	if (item_cb != NULL) {
		lv_obj_add_event_cb(btn, item_cb, LV_EVENT_CLICKED, NULL);
	}
}

void ui_list_set_item_click_cb(lv_obj_t *list, lv_event_cb_t cb)
{
	lv_obj_set_user_data(list, (void *)cb);
}

lv_obj_t *ui_list_create(lv_obj_t *parent, const char **items, size_t count)
{
	lv_obj_t *list = lv_list_create(parent);

	ui_list_set_items(list, items, count);

	return list;
}

void ui_list_set_items(lv_obj_t *list, const char **items, size_t count)
{
	lv_obj_clean(list);

	if (count == 0) {
		lv_list_add_text(list, "No networks found");
		return;
	}

	for (size_t i = 0; i < count; i++) {
		add_item(list, items[i]);
	}
}
