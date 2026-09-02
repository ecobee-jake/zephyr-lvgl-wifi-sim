/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifi-sta.h"

static struct net_mgmt_event_callback wifi_cb;

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

static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t event,
                               struct net_if *iface)
{
    if (event == NET_EVENT_WIFI_SCAN_RESULT) {
        const struct wifi_scan_result *result = cb->info;

        printk("SSID: %.*s RSSI: %d\n",
               result->ssid_length,
               result->ssid,
               result->rssi);
    }

    if (event == NET_EVENT_WIFI_SCAN_DONE) {
        printk("scan done\n");
    }
}