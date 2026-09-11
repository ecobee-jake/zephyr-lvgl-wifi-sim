/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "wifi-sta.h"
#include "../app_events.h"

#if defined(CONFIG_WIFI)

#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/net_ip.h>

/* Hardcoded AP from scripts/bring-up-dev-iface.sh (same creds as zephyr's
 * tests/net/wifi/interop). Upstream is proved by pinging past the NAT.
 */
#define WIFI_STA_SSID       "zephyr-wpa2"
#define WIFI_STA_PSK        "password"
#define UPSTREAM_PING_ADDR  "8.8.8.8"
#define UPSTREAM_PING_TRIES 3
#define UPSTREAM_PING_WAIT  K_SECONDS(2)

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static struct wifi_sta_scan_entry scan_results[WIFI_STA_SCAN_MAX_RESULTS];
static size_t scan_result_count;

static struct net_icmp_ctx icmp_ctx;
static K_SEM_DEFINE(ping_reply_sem, 0, 1);

static enum net_verdict ping_reply_handler(struct net_icmp_ctx *ctx,
                                           struct net_pkt *pkt,
                                           struct net_icmp_ip_hdr *ip_hdr,
                                           struct net_icmp_hdr *icmp_hdr,
                                           void *user_data)
{
    ARG_UNUSED(ctx);
    ARG_UNUSED(pkt);
    ARG_UNUSED(ip_hdr);
    ARG_UNUSED(icmp_hdr);
    ARG_UNUSED(user_data);

    k_sem_give(&ping_reply_sem);

    return NET_OK;
}

/* Runs off the system workqueue: the ping loop blocks, so it must not run in
 * the net_mgmt event callback.
 */
static void upstream_ping_work(struct k_work *work)
{
    struct net_if *iface = net_if_get_wifi_sta();
    struct net_sockaddr_in dst = { .sin_family = AF_INET };
    struct net_icmp_ping_params params = { 0 };

    ARG_UNUSED(work);

    if (net_addr_pton(AF_INET, UPSTREAM_PING_ADDR, &dst.sin_addr) < 0) {
        printk("UPSTREAM FAIL: bad address " UPSTREAM_PING_ADDR "\n");
        return;
    }

    for (int i = 1; i <= UPSTREAM_PING_TRIES; i++) {
        int ret;

        params.identifier = 1;
        params.sequence = i;

        k_sem_reset(&ping_reply_sem);

        ret = net_icmp_send_echo_request(&icmp_ctx, iface,
                                         (struct net_sockaddr *)&dst,
                                         &params, NULL);
        if (ret < 0) {
            printk("ping %d/%d: send failed (%d)\n", i, UPSTREAM_PING_TRIES, ret);
            continue;
        }

        if (k_sem_take(&ping_reply_sem, UPSTREAM_PING_WAIT) == 0) {
            printk("ping %d/%d: reply from " UPSTREAM_PING_ADDR "\n",
                   i, UPSTREAM_PING_TRIES);
            printk("UPSTREAM OK\n");
            return;
        }

        printk("ping %d/%d: timeout\n", i, UPSTREAM_PING_TRIES);
    }

    printk("UPSTREAM FAIL\n");
}

static K_WORK_DEFINE(ping_work, upstream_ping_work);

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

    if (event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *status = cb->info;

        if (status->status != 0) {
            printk("connect failed (%d)\n", status->status);
            return;
        }

        printk("connected to " WIFI_STA_SSID ", requesting DHCP\n");
        net_dhcpv4_start(net_if_get_wifi_sta());
    }

    if (event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        printk("disconnected\n");
    }
}

static void ipv4_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t event,
                               struct net_if *iface)
{
    ARG_UNUSED(cb);

    if (event == NET_EVENT_IPV4_DHCP_BOUND) {
        struct net_in_addr *addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
        char buf[NET_IPV4_ADDR_LEN];

        printk("DHCP bound: %s\n",
               addr ? net_addr_ntop(AF_INET, addr, buf, sizeof(buf)) : "?");

        k_work_submit(&ping_work);
    }
}

void wifi_sta_init(void)
{
    int ret;

    net_mgmt_init_event_callback(
        &wifi_cb,
        wifi_event_handler,
        NET_EVENT_WIFI_SCAN_RESULT |
        NET_EVENT_WIFI_SCAN_DONE |
        NET_EVENT_WIFI_CONNECT_RESULT |
        NET_EVENT_WIFI_DISCONNECT_RESULT);

    net_mgmt_add_event_callback(&wifi_cb);

    net_mgmt_init_event_callback(
        &ipv4_cb,
        ipv4_event_handler,
        NET_EVENT_IPV4_DHCP_BOUND);

    net_mgmt_add_event_callback(&ipv4_cb);

    ret = net_icmp_init_ctx(&icmp_ctx, AF_INET, NET_ICMPV4_ECHO_REPLY, 0,
                            ping_reply_handler);
    if (ret < 0) {
        printk("icmp ctx init failed (%d)\n", ret);
    }
}

void wifi_sta_connect(void)
{
    struct net_if *iface = net_if_get_wifi_sta();
    struct wifi_connect_req_params params = {
        .ssid = (const uint8_t *)WIFI_STA_SSID,
        .ssid_length = sizeof(WIFI_STA_SSID) - 1,
        .psk = (const uint8_t *)WIFI_STA_PSK,
        .psk_length = sizeof(WIFI_STA_PSK) - 1,
        .security = WIFI_SECURITY_TYPE_PSK,
        .channel = WIFI_CHANNEL_ANY,
        .band = WIFI_FREQ_BAND_UNKNOWN,
        .mfp = WIFI_MFP_OPTIONAL,
    };

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));

    printk("connect request (" WIFI_STA_SSID "): %d\n", ret);
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

#else /* !CONFIG_WIFI */

/* Base build: no Wi-Fi stack to talk to. The provisioning screen still builds and
 * opens, its scan list just stays empty.
 */
void wifi_sta_init(void) { }
void wifi_sta_scan(void) { }
void wifi_sta_connect(void) { }
void wifi_sta_handle_event(const struct app_event *evt) { ARG_UNUSED(evt); }

size_t wifi_sta_get_scan_results(const struct wifi_sta_scan_entry **results)
{
    *results = NULL;
    return 0;
}

#endif /* CONFIG_WIFI */
