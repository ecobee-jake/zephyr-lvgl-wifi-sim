# Handoff — native_sim WiFi scan on Zephyr

## Status: DONE (current goal met)
Zephyr WiFi API scans **virtual** APs on native_sim. `wifi-sta.c` issues
`NET_REQUEST_WIFI_SCAN` and receives `NET_EVENT_WIFI_SCAN_RESULT` (SSID
`zephyr-open`, RSSI -30) then `NET_EVENT_WIFI_SCAN_DONE`, via jukkar's PR #110681
driver → wpa_supplicant → host nl80211 → **mac80211_hwsim** virtual radios.

Working tree: `/home/jake/firmware/zephyr-test/lvgl-demo`. Do not re-derive the
build — it works. Scripts already exist:
- `scripts/build.sh` (sets i386 PKG_CONFIG_PATH, `west build`)
- `scripts/run.sh` (setcap, `nmcli set <iface> managed no`, run + tee log)
- `scripts/build-and-run.sh`
- `scripts/bring-up-dev-iface.sh` / `clean-up-dev-iface.sh` (host radio setup)

Key config already in place (do not rediscover): `west.yml` uses jukkar fork
`devel/wifi-driver-for-native_sim`, allowlist includes `mbedtls tf-psa-crypto
hostap picolibc`. Board = `native_sim` (32-bit, NOT native/64). DT overlay node
`zephyr,native-sim-wifi` with `host-interface = "zwifi"`. `CONFIG_ETH_NATIVE_TAP=n`,
picolibc auto-selected. Full build history is in the prior transcript.

## Current binding
- `zwifi` = hwsim radio the Zephyr driver binds to (DT `host-interface`).
- `zap` = hwsim radio running hostapd (`zephyr-open`), the fake AP being scanned.
- `wlp0s20f3` = the **real** Intel card. Currently **untouched**.

## NEXT GOAL (do NOT attempt now — just context)
"Scan **real** APs via the iface on my dev machine" — i.e. point the driver at
`wlp0s20f3` (or another real radio) so the Zephyr scan returns the actual APs in
the room instead of the hwsim `zephyr-open`.

### Why this is NOT a one-liner (critical context)
- PR #110681 states it is "**not** a driver for any real hardware"; behavior
  against a physical card (iwlwifi/wlp0s20f3) is **explicitly unestablished**.
- The driver takes over the host interface via nl80211 + AF_PACKET and expects
  wpa_supplicant to be the sole SME. Pointing it at the real card means Zephyr's
  supplicant fights the host's NetworkManager/wpa_supplicant for that radio, and
  would drop your real WiFi connection.
- A scan on a real card is a normal nl80211 trigger + get_scan; the open
  question is whether the PR's host-thread/eloop/socketpair path works on a real
  phy and whether it can scan **without** claiming the interface (managed-off /
  monitor / a second vif) so you don't lose connectivity.
- Simplest safe path to explore first: change DT `host-interface` to the real
  iface, run only the scan (no connect), and decide how to stop NM from
  reclaiming it. Consider a dedicated monitor/second vif rather than the primary.

### Approach discipline (user's explicit instruction)
Do NOT try to do this all at once. Break it into steps, confirm each with the
user (he runs build/test himself and pastes output), propose one fix at a time.

## Guardrails carried from prior session
- Do not edit `src/wifi-sta.c`.
- Do not build/test yourself; tell the user when to build/run.
- Do not open driver `.c` files unless a build error names one.
- Host setup is not persistent (reboot / killed terminal wipes it). Run hostapd
  in a plain terminal, not VSCode's (force-kill takes it down). `setcap` is wiped
  by every relink — `run.sh` re-applies it.

## Suggested skills for next agent
- `superpowers:brainstorming` — the next goal is exploratory/architectural
  (unproven on real hardware); design the approach + get approval before acting.
- `superpowers:systematic-debugging` — once real-scan attempts fail, root-cause
  before fixes (multi-component: NM ↔ nl80211 ↔ driver ↔ supplicant).
