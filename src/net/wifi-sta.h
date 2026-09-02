/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_WIFI_STA_H_
#define APP_WIFI_STA_H_

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

void wifi_sta_init(void);


static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t event,
                               struct net_if *iface);

void wifi_sta_scan(void);

#endif /* APP_WIFI_STA_H_ */
