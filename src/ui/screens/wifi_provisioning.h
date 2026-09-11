/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_UI_SCREENS_WIFI_PROVISIONING_H_
#define APP_UI_SCREENS_WIFI_PROVISIONING_H_

#include <lvgl.h>

lv_obj_t *screen_wifi_provisioning_create(void);
void screen_wifi_provisioning_refresh_scan_results(void);

#endif /* APP_UI_SCREENS_WIFI_PROVISIONING_H_ */
