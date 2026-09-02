/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui.h"
#include "router.h"

void ui_init(void)
{
	router_init();
}

void ui_handle_event(const struct app_event *evt)
{
	router_handle_event(evt);
}
