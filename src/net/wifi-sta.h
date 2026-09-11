/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_WIFI_STA_H_
#define APP_WIFI_STA_H_

#include <stddef.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

#define WIFI_STA_SCAN_MAX_RESULTS 32

struct wifi_sta_scan_entry {
	char ssid[WIFI_SSID_MAX_LEN + 1];
	int8_t rssi;
};

void wifi_sta_init(void);
void wifi_sta_scan(void);

/* Connects to the hardcoded WPA2 AP from scripts/bring-up-dev-iface.sh. */
void wifi_sta_connect(void);

/* Returns a pointer to the most recent scan results and their count. */
size_t wifi_sta_get_scan_results(const struct wifi_sta_scan_entry **results);

#endif /* APP_WIFI_STA_H_ */
