/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <lvgl_zephyr.h>

#include "app_events.h"
#include "ui/ui.h"
#include "net/wifi-sta.h"
#include "diag/heap-monitor.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define UI_THREAD_STACK_SIZE 4096
#define UI_THREAD_PRIORITY   5

#define EVENT_PRODUCER_STACK_SIZE 1024
#define EVENT_PRODUCER_PRIORITY   7

K_MSGQ_DEFINE(event_queue, sizeof(struct app_event), 4, 4);

void app_event_post(enum app_event_id id)
{
	struct app_event evt = { .id = id };

	k_msgq_put(&event_queue, &evt, K_NO_WAIT);
}

static struct k_timer tick_timer;

static void tick_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	app_event_post(APP_EVENT_TICK);
}

static void event_producer_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_timer_init(&tick_timer, tick_timer_expiry, NULL);
	k_timer_start(&tick_timer, K_SECONDS(1), K_SECONDS(1));
}

static void ui_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *display_dev;
	int ret;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return;
	}

	lvgl_lock();
	ui_init();
	lv_timer_handler();
	lvgl_unlock();

	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to turn blanking off (error %d)", ret);
		return;
	}

	while (1) {
		struct app_event evt;
		uint32_t sleep_ms;

		lvgl_lock();
		if (k_msgq_get(&event_queue, &evt, K_NO_WAIT) == 0) {
			switch (evt.id)
			{
			case APP_EVENT_TICK:
				heap_monitor_on_tick();
				break;
			case APP_EVENT_GOTO_HOME:
				ui_handle_event(&evt);
				break;
			case APP_EVENT_GOTO_WIFI_PROVISIONING:
				ui_handle_event(&evt);
				break;
			case APP_EVENT_WIFI_SCAN_REQUEST:
				wifi_sta_scan();
				break;
			case APP_EVENT_WIFI_CONNECT_REQUEST:
				wifi_sta_connect();
				break;
			case APP_EVENT_WIFI_SCAN_DONE:
				ui_handle_event(&evt);
				break;

			default:
				break;
			}
		}
		sleep_ms = lv_timer_handler();
		lvgl_unlock();

		k_msleep(MIN(sleep_ms, INT32_MAX));
	}
}

K_THREAD_DEFINE(ui_tid, UI_THREAD_STACK_SIZE, ui_thread, NULL, NULL, NULL,
		 UI_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(event_producer_tid, EVENT_PRODUCER_STACK_SIZE, event_producer_thread,
		 NULL, NULL, NULL, EVENT_PRODUCER_PRIORITY, 0, 0);

int main(void)
{
	heap_monitor_init();
	wifi_sta_init();

	return 0;
}
