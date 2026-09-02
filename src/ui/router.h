/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_UI_ROUTER_H_
#define APP_UI_ROUTER_H_

#include "../app_events.h"

enum ui_screen_id {
	UI_SCREEN_HOME,
	UI_SCREEN_WIFI_PROVISIONING,
};

void router_init(void);
void router_goto(enum ui_screen_id id);
void router_handle_event(const struct app_event *evt);

#endif /* APP_UI_ROUTER_H_ */
