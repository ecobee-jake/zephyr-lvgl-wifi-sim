/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "button.h"

lv_obj_t *ui_button_create(lv_obj_t *parent, const char *text)
{
	lv_obj_t *btn = lv_button_create(parent);
	lv_obj_t *label = lv_label_create(btn);

	lv_label_set_text(label, text);
	lv_obj_center(label);

	return btn;
}
