/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_UI_COMPONENTS_LIST_H_
#define APP_UI_COMPONENTS_LIST_H_

#include <lvgl.h>
#include <stddef.h>

/* Always single-select: checking one item unchecks the others. */
lv_obj_t *ui_list_create(lv_obj_t *parent, const char **items, size_t count);

#endif /* APP_UI_COMPONENTS_LIST_H_ */
