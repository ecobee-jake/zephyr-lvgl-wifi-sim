/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_HEAP_MONITOR_H_
#define APP_HEAP_MONITOR_H_

#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/sys_heap.h>

/*
 * Periodic heap reporting. Each heap is registered as its own source and
 * reported on its own line - the kernel keeps no name for a sys_heap, so the
 * name comes from whoever registers it.
 *
 * Sampling is driven by APP_EVENT_TICK (see heap_monitor_on_tick()), so this
 * module owns no timer or thread of its own.
 */

#define HEAP_MONITOR_MAX_SOURCES 4

/* Fills in @stats for one heap. Returns 0 on success, a negative errno
 * otherwise (a failing source is reported, not silently skipped).
 */
typedef int (*heap_monitor_stats_fn)(struct sys_memory_stats *stats, void *ctx);

/* Registers a heap fronted by a stats accessor rather than a sys_heap pointer,
 * as LVGL's private pool is. @name must outlive the registration.
 */
int heap_monitor_register(const char *name, heap_monitor_stats_fn fn, void *ctx);

/* Registers a plain sys_heap (covers k_heap too, via its .heap member). */
int heap_monitor_register_sys_heap(const char *name, struct sys_heap *heap);

/* Registers the heaps known at startup and logs what is being monitored. */
void heap_monitor_init(void);

/* Advances the sample counter; samples and logs every Nth tick. */
void heap_monitor_on_tick(void);

#endif /* APP_HEAP_MONITOR_H_ */
