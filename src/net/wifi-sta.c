/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/util.h>

#include "wifi-sta.h"
#include "../app_events.h"

static struct net_mgmt_event_callback wifi_cb;

static struct wifi_sta_scan_entry scan_results[WIFI_STA_SCAN_MAX_RESULTS];
static size_t scan_result_count;

static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t event,
                               struct net_if *iface)
{
    ARG_UNUSED(iface);

    if (event == NET_EVENT_WIFI_SCAN_RESULT) {
        const struct wifi_scan_result *result = cb->info;

        printk("SSID: %.*s RSSI: %d\n",
               result->ssid_length,
               result->ssid,
               result->rssi);

        if (scan_result_count < WIFI_STA_SCAN_MAX_RESULTS) {
            struct wifi_sta_scan_entry *entry = &scan_results[scan_result_count];
            size_t len = MIN(result->ssid_length, WIFI_SSID_MAX_LEN);

            memcpy(entry->ssid, result->ssid, len);
            entry->ssid[len] = '\0';
            entry->rssi = result->rssi;

            scan_result_count++;
        }
    }

    if (event == NET_EVENT_WIFI_SCAN_DONE) {
        printk("scan done\n");
        app_event_post(APP_EVENT_WIFI_SCAN_DONE);
    }
}

void wifi_sta_init(void)
{

    net_mgmt_init_event_callback(
        &wifi_cb,
        wifi_event_handler,
        NET_EVENT_WIFI_SCAN_RESULT |
        NET_EVENT_WIFI_SCAN_DONE);

    net_mgmt_add_event_callback(&wifi_cb);
}


void wifi_sta_scan(void)
{
    struct net_if *iface = net_if_get_wifi_sta();

    scan_result_count = 0;

    int ret = net_mgmt(
        NET_REQUEST_WIFI_SCAN,
        iface,
        NULL,
        0);

    printk("scan request: %d\n", ret);

}

void wifi_sta_handle_event(const struct app_event *evt)
{
	// Handle Wi-Fi STA specific events here
}

size_t wifi_sta_get_scan_results(const struct wifi_sta_scan_entry **results)
{
    *results = scan_results;
    return scan_result_count;
}
