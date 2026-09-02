/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#include "wifi_provisioning.h"
#include "../../app_events.h"
#include "../components/button.h"
#include "../components/list.h"

static const char *networks[] = {
	"Home-WiFi",
	"Office-5G",
	"Guest",
};

static void scan_click_cb(lv_event_t *e)
{
	(void)e;

	app_event_post(APP_EVENT_WIFI_SCAN_REQUEST);
}

static void back_click_cb(lv_event_t *e)
{
	(void)e;

	app_event_post(APP_EVENT_GOTO_HOME);
}

lv_obj_t *screen_wifi_provisioning_create(void)
{
	lv_obj_t *screen = lv_obj_create(NULL);

	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	ui_list_create(screen, networks, ARRAY_SIZE(networks));

	lv_obj_t *scan_btn = ui_button_create(screen, "Scan");

	lv_obj_add_event_cb(scan_btn, scan_click_cb, LV_EVENT_CLICKED, NULL);

	lv_obj_t *back_btn = ui_button_create(screen, "Back");

	lv_obj_add_event_cb(back_btn, back_click_cb, LV_EVENT_CLICKED, NULL);

	return screen;
}
