/*
 * Fake WiFi driver for native_sim: native_sim has no real/simulated WiFi
 * radio, so net_if_get_wifi_sta() finds nothing and scan requests fail with
 * -ENOTSUP. This registers a software-only net_if flagged as WiFi (same
 * mechanism Zephyr's own tests/net/wifi/wifi_nm test uses) so the WiFi mgmt
 * API has a real interface and driver to dispatch scan requests to.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi_mgmt.h>

struct wifi_fake_dev_data {
	uint8_t mac_addr[6];
};

static struct wifi_fake_dev_data wifi_fake_data;

static const char *const fake_ssids[] = {
	"Home-WiFi",
	"Office-5G",
	"Guest",
};

static void wifi_fake_iface_init(struct net_if *iface)
{
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	struct wifi_fake_dev_data *data = net_if_get_device(iface)->data;

	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x00;
	data->mac_addr[2] = 0x5E;
	data->mac_addr[3] = 0x00;
	data->mac_addr[4] = 0x53;
	data->mac_addr[5] = 0x01;

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;

	ethernet_init(iface);
}

static int wifi_fake_scan(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb)
{
	struct net_if *iface = net_if_lookup_by_dev(dev);

	ARG_UNUSED(params);

	for (size_t i = 0; i < ARRAY_SIZE(fake_ssids); i++) {
		struct wifi_scan_result result = {
			.channel = 6,
			.security = WIFI_SECURITY_TYPE_PSK,
			.rssi = (int8_t)(-40 - i * 10),
		};

		result.ssid_length = MIN(strlen(fake_ssids[i]), sizeof(result.ssid));
		memcpy(result.ssid, fake_ssids[i], result.ssid_length);

		cb(iface, 0, &result);
	}

	cb(iface, 0, NULL);

	return 0;
}

static const struct wifi_mgmt_ops wifi_fake_mgmt_api = {
	.scan = wifi_fake_scan,
};

static const struct net_wifi_mgmt_offload wifi_fake_api = {
	.wifi_iface.iface_api.init = wifi_fake_iface_init,
	.wifi_mgmt_api = &wifi_fake_mgmt_api,
};

ETH_NET_DEVICE_INIT(wifi_fake, "wifi_fake", NULL, NULL, &wifi_fake_data, NULL,
		     CONFIG_ETH_INIT_PRIORITY, &wifi_fake_api, NET_ETH_MTU);
