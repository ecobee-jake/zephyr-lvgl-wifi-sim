/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>

#include "router.h"
#include "screens/home.h"
#include "screens/wifi_provisioning.h"

void router_goto(enum ui_screen_id id)
{
	lv_obj_t *prev = lv_screen_active();
	lv_obj_t *next;

	switch (id) {
	case UI_SCREEN_WIFI_PROVISIONING:
		next = screen_wifi_provisioning_create();
		break;
	case UI_SCREEN_HOME:
	default:
		next = screen_home_create();
		break;
	}

	lv_screen_load(next);
	lv_obj_delete(prev);
}

void router_init(void)
{
	lv_obj_t *prev = lv_screen_active();

	lv_screen_load(screen_home_create());
	lv_obj_delete(prev);
}

void router_handle_event(const struct app_event *evt)
{
	switch (evt->id) {
	case APP_EVENT_GOTO_HOME:
		router_goto(UI_SCREEN_HOME);
		break;
	case APP_EVENT_GOTO_WIFI_PROVISIONING:
		router_goto(UI_SCREEN_WIFI_PROVISIONING);
		break;
	default:
		break;
	}
}
