/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_EVENTS_H_
#define APP_EVENTS_H_

enum app_event_id {
	APP_EVENT_TICK,
	APP_EVENT_GOTO_HOME,
	APP_EVENT_GOTO_WIFI_PROVISIONING,
	APP_EVENT_WIFI_SCAN_RESULT,
	APP_EVENT_WIFI_SCAN_DONE,
	APP_EVENT_WIFI_SCAN_REQUEST,
	APP_EVENT_WIFI_CONNECT_REQUEST
};

struct app_event {
	enum app_event_id id;
};

void app_event_post(enum app_event_id id);

#endif /* APP_EVENTS_H_ */
