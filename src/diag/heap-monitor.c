/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/kernel.h>

#ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
#include <lvgl_mem.h>
#endif

#include "heap-monitor.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(heap_monitor);

/* Sample every Nth APP_EVENT_TICK. tick_timer in main.c runs at 1 Hz today, so
 * this is 5s - retuning that timer retunes this too, by design.
 */
#define HEAP_MONITOR_TICK_INTERVAL 5

struct heap_monitor_source {
	const char *name;
	heap_monitor_stats_fn fn;
	void *ctx;
	/* Set when registered by pointer, so discovery can tell that an
	 * enumerated heap already has a name. NULL for accessor-backed heaps.
	 */
	struct sys_heap *heap;
};

static struct heap_monitor_source sources[HEAP_MONITOR_MAX_SOURCES];
static size_t source_count;
static uint32_t tick_count;

int heap_monitor_register(const char *name, heap_monitor_stats_fn fn, void *ctx)
{
	if (name == NULL || fn == NULL) {
		return -EINVAL;
	}

	if (source_count >= ARRAY_SIZE(sources)) {
		LOG_ERR("cannot register '%s': at HEAP_MONITOR_MAX_SOURCES (%d)", name,
			HEAP_MONITOR_MAX_SOURCES);
		return -ENOMEM;
	}

	sources[source_count].name = name;
	sources[source_count].fn = fn;
	sources[source_count].ctx = ctx;
	sources[source_count].heap = NULL;
	source_count++;

	return 0;
}

static int sys_heap_source_stats(struct sys_memory_stats *stats, void *ctx)
{
	return sys_heap_runtime_stats_get((struct sys_heap *)ctx, stats);
}

int heap_monitor_register_sys_heap(const char *name, struct sys_heap *heap)
{
	int ret;

	if (heap == NULL) {
		return -EINVAL;
	}

	ret = heap_monitor_register(name, sys_heap_source_stats, heap);
	if (ret == 0) {
		sources[source_count - 1].heap = heap;
	}

	return ret;
}

#ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
/* LVGL's pool is a file-static sys_heap, reachable only through this accessor. */
static int lvgl_source_stats(struct sys_memory_stats *stats, void *ctx)
{
	ARG_UNUSED(ctx);

	lvgl_heap_stats(stats);

	return 0;
}
#endif /* CONFIG_LV_Z_MEM_POOL_SYS_HEAP */

/* Logs one heap, or nothing if it has not been initialised yet. Returns the
 * usable total, which is 0 for an uninitialised heap.
 */
static size_t heap_monitor_log(const char *name, const struct sys_memory_stats *stats)
{
	/* Usable size, so slightly under the configured pool size: chunk headers
	 * and the heap's own bookkeeping are not counted here.
	 */
	size_t total = stats->allocated_bytes + stats->free_bytes;

	if (total == 0) {
		/* Not initialised yet - LVGL's pool comes up on the UI thread,
		 * which may lag the first ticks.
		 */
		LOG_DBG("%s: not initialised yet", name);
		return 0;
	}

	LOG_INF("%s: used=%zu total=%zu (%zu%%) free=%zu peak=%zu", name,
		stats->allocated_bytes, total, (stats->allocated_bytes * 100) / total,
		stats->free_bytes, stats->max_allocated_bytes);

	return total;
}

#if CONFIG_SYS_HEAP_ARRAY_SIZE > 0
/* True when an enumerated heap is already covered by a registered source.
 *
 * Pointer-registered sources match directly. Accessor-backed sources (LVGL) have
 * no pointer to compare, so they are matched on their stats instead: sampling
 * runs on the UI thread under lvgl_lock(), so LVGL's pool cannot change
 * underneath us and the triple is an exact comparison, not a heuristic.
 */
static bool heap_is_registered(struct sys_heap *heap, const struct sys_memory_stats *stats)
{
	for (size_t i = 0; i < source_count; i++) {
		const struct heap_monitor_source *src = &sources[i];
		struct sys_memory_stats named;

		if (src->heap != NULL) {
			if (src->heap == heap) {
				return true;
			}
			continue;
		}

		if (src->fn(&named, src->ctx) != 0) {
			continue;
		}

		/* Skip the all-zero case: two uninitialised heaps would other-
		 * wise look identical to each other.
		 */
		if (named.allocated_bytes + named.free_bytes == 0) {
			continue;
		}

		if (named.allocated_bytes == stats->allocated_bytes &&
		    named.free_bytes == stats->free_bytes &&
		    named.max_allocated_bytes == stats->max_allocated_bytes) {
			return true;
		}
	}

	return false;
}

/* Reports every sys_heap in the image that no source claimed. Every heap self-
 * registers from sys_heap_init(), so this covers the kernel system heap and the
 * libc arena, neither of which exposes its sys_heap for registration.
 *
 * Re-run each sample rather than cached: heaps appear as their owners
 * initialise, and at a handful of heaps every 5s the cost is irrelevant.
 */
static void heap_monitor_sample_discovered(void)
{
	struct sys_heap **heaps;
	int count;

	count = sys_heap_array_get(&heaps);
	if (count < 0) {
		LOG_WRN("heap enumeration failed (%d)", count);
		return;
	}

	if (count == CONFIG_SYS_HEAP_ARRAY_SIZE) {
		/* sys_heap_init() ignores sys_heap_array_save()'s -ENOMEM, so an
		 * overflow is otherwise silent and heaps just go missing.
		 */
		LOG_WRN_ONCE("heap array full (%d) - some heaps may be unreported, "
			     "raise CONFIG_SYS_HEAP_ARRAY_SIZE",
			     CONFIG_SYS_HEAP_ARRAY_SIZE);
	}

	for (int i = 0; i < count; i++) {
		struct sys_memory_stats stats;
		char name[24];

		if (sys_heap_runtime_stats_get(heaps[i], &stats) != 0) {
			continue;
		}

		if (heap_is_registered(heaps[i], &stats)) {
			continue;
		}

		/* The kernel keeps no name for a sys_heap, so index and address
		 * are all there is to identify an unregistered one by.
		 */
		snprintf(name, sizeof(name), "heap[%d] @%p", i, (void *)heaps[i]);
		(void)heap_monitor_log(name, &stats);
	}
}
#endif /* CONFIG_SYS_HEAP_ARRAY_SIZE > 0 */

static void heap_monitor_sample(void)
{
	for (size_t i = 0; i < source_count; i++) {
		const struct heap_monitor_source *src = &sources[i];
		struct sys_memory_stats stats;
		int ret;

		ret = src->fn(&stats, src->ctx);
		if (ret != 0) {
			LOG_WRN("%s: stats unavailable (%d)", src->name, ret);
			continue;
		}

		(void)heap_monitor_log(src->name, &stats);
	}

#if CONFIG_SYS_HEAP_ARRAY_SIZE > 0
	heap_monitor_sample_discovered();
#endif
}

void heap_monitor_init(void)
{
	tick_count = 0;

#ifdef CONFIG_LV_Z_MEM_POOL_SYS_HEAP
	(void)heap_monitor_register("lvgl", lvgl_source_stats, NULL);
#endif

	/* Only the registered count is known here: the discovered heaps are
	 * enumerated per sample, and LVGL's pool does not exist yet - lv_mem_init()
	 * runs later, on the UI thread.
	 */
	LOG_INF("monitoring %zu named heap(s) + discovered heaps every %d tick(s)", source_count,
		HEAP_MONITOR_TICK_INTERVAL);
}

void heap_monitor_on_tick(void)
{
	if (++tick_count < HEAP_MONITOR_TICK_INTERVAL) {
		return;
	}

	tick_count = 0;
	heap_monitor_sample();
}
