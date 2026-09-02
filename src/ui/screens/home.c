/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "home.h"
#include "../../app_events.h"
#include "../components/button.h"

static void wifi_setup_click_cb(lv_event_t *e)
{
	(void)e;

	app_event_post(APP_EVENT_GOTO_WIFI_PROVISIONING);
}

lv_obj_t *screen_home_create(void)
{
	lv_obj_t *screen = lv_obj_create(NULL);

	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t *label = lv_label_create(screen);

	lv_label_set_text(label, "Hello, World!");

	lv_obj_t *btn = ui_button_create(screen, "Wi-Fi Setup");

	lv_obj_add_event_cb(btn, wifi_setup_click_cb, LV_EVENT_CLICKED, NULL);

	return screen;
}
