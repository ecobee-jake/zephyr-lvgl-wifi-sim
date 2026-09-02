# lvgl-demo

A Zephyr `native_sim` app used to learn Zephyr/LVGL concepts (threading, display, event-driven UI) that transfer to a real embedded device with a display and WiFi/BLE.

## Language

**App Event**:
A `struct app_event` posted to the app-wide `k_msgq` (`event_queue`) via `app_event_post()`. Can be produced from any thread (currently only the 1s tick producer thread) and is consumed by the UI thread's loop, which hands it to `ui_handle_event()`. This is the mechanism a future BLE/WiFi thread would use to notify the UI thread.
_Avoid_: Event (ambiguous with UI Event), message.

**UI Event**:
An LVGL `lv_event_t` delivered synchronously to an `lv_obj_add_event_cb()` callback (e.g. `LV_EVENT_CLICKED`), always handled on the UI thread that called `lv_timer_handler()`. Screens react to these directly (e.g. by calling `app_event_post()`).
_Avoid_: Event (ambiguous with App Event), callback.

**Router**:
Owns the single active LVGL screen and screen-switching (`router_goto()`), and interprets App Events that request navigation (`router_handle_event()`). Lives in `src/ui/router.c`.
