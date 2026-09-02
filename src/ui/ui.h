/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_UI_H_
#define APP_UI_H_

#include "../app_events.h"

void ui_init(void);
void ui_handle_event(const struct app_event *evt);

#endif /* APP_UI_H_ */
