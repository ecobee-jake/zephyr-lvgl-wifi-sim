/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>

#include "wifi_provisioning.h"
#include "../../app_events.h"
#include "../../net/wifi-sta.h"
#include "../components/button.h"
#include "../components/list.h"

static lv_obj_t *network_list;

static void scan_click_cb(lv_event_t *e)
{
	(void)e;

	app_event_post(APP_EVENT_WIFI_SCAN_REQUEST);
}

/* Credentials are hardcoded, so any row connects to the same AP. Carrying the
 * pressed row's SSID would need a payload on struct app_event.
 */
static void network_click_cb(lv_event_t *e)
{
	(void)e;

	app_event_post(APP_EVENT_WIFI_CONNECT_REQUEST);
}

static void back_click_cb(lv_event_t *e)
{
	(void)e;

	app_event_post(APP_EVENT_GOTO_HOME);
}

static void screen_delete_cb(lv_event_t *e)
{
	(void)e;

	network_list = NULL;
}

void screen_wifi_provisioning_refresh_scan_results(void)
{
	const struct wifi_sta_scan_entry *results;
	size_t count = wifi_sta_get_scan_results(&results);
	const char *names[WIFI_STA_SCAN_MAX_RESULTS];

	if (network_list == NULL) {
		return;
	}

	for (size_t i = 0; i < count; i++) {
		names[i] = results[i].ssid[0] != '\0' ? results[i].ssid : "(Hidden Network)";
	}

	ui_list_set_items(network_list, names, count);
}

lv_obj_t *screen_wifi_provisioning_create(void)
{
	lv_obj_t *screen = lv_obj_create(NULL);

	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	network_list = ui_list_create(screen, NULL, 0);
	ui_list_set_item_click_cb(network_list, network_click_cb);
	lv_obj_add_event_cb(screen, screen_delete_cb, LV_EVENT_DELETE, NULL);

	lv_obj_t *scan_btn = ui_button_create(screen, "Scan");

	lv_obj_add_event_cb(scan_btn, scan_click_cb, LV_EVENT_CLICKED, NULL);

	lv_obj_t *back_btn = ui_button_create(screen, "Back");

	lv_obj_add_event_cb(back_btn, back_click_cb, LV_EVENT_CLICKED, NULL);

	return screen;
}
