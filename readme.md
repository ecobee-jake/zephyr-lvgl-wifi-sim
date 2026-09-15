## One-time, per machine:

Pick any empty dir as the west workspace; the repo is cloned *inside* it.

`mkdir -p zephyr-workspace && cd zephyr-workspace`

`git clone git@github.com:ecobee-jake/zephyr-lvgl-wifi-sim.git`

`python3 -m venv .venv && source .venv/bin/activate  # Python 3.12 recommended`

`pip install west`

`west init -l zephyr-lvgl-wifi-sim          # makes zephyr-lvgl-wifi-sim the manifest repo`

`west update                     # clones zephyr/ and modules/ as siblings`

`pip install -r zephyr/scripts/requirements.txt`

`west zephyr-export`

`cd zephyr-lvgl-wifi-sim/`

`pip install cmake`

`sudo apt install libsdl2-dev:i386`

*You should be able to run the "Base: LVGL only, no networking" simulator below here... Which doesn't do much but proves build env, if you build run `rm -rf build` afterwards*

`sudo dpkg --add-architecture i386 && sudo apt update`

`sudo apt install libnl-3-dev:i386 libnl-genl-3-dev:i386 gcc-multilib pkg-config hostapd`

`export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig`

*You should be able to run the "Run zephyr sim with simulated Linux network interface." simulator below*





## Two Zephyrs

The Wi-Fi work needs a fork (jukkar's `devel/wifi-driver-for-native_sim`, PR #110681)
because upstream has no native_sim Wi-Fi driver. The base build uses plain upstream
v4.4.2. Only one is checked out at a time.

```
./scripts/switch-zephyr.sh          # which am I on?
./scripts/switch-zephyr.sh base     # upstream v4.4.2, LVGL only
./scripts/switch-zephyr.sh wifi     # the fork
```

Switching runs `west update` and deletes `build/`.


## Base: LVGL only, no networking
Needs `switch-zephyr.sh base`. The Wi-Fi screen still opens; its scan list is empty.
```
./scripts/switch-zephyr.sh base     # upstream v4.4.2, LVGL only

./scripts/build-and-run.sh -p -b native_sim
```


## Run zephyr sim with *simulated* Linux network interface.
Needs `switch-zephyr.sh wifi`. Zephyr sim can scan a simulated AP, STA connect, and
ping an upstream IP. Your real Wi-Fi is untouched.
```
./scripts/switch-zephyr.sh wifi  # Don't need to do this each build

sudo ./scripts/bring-up-dev-iface.sh  # configure simulated AP

./scripts/build-and-run.sh -p -b native_sim -- -DEXTRA_DTC_OVERLAY_FILE=overlays/dev-iface.overlay -DEXTRA_CONF_FILE=overlays/wifi.conf

sudo ./scripts/clean-up-dev-iface.sh
```

## Run zephyr sim with real Linux network interface.
Needs `switch-zephyr.sh wifi`. Zephyr sim can scan for real APs, but is not able to
connect to upstream.

**Warning:** this hands your laptop's Wi-Fi card to Zephyr, so your internet
connection drops until you run the clean-up script.

```
./scripts/switch-zephyr.sh wifi   # Don't need to do this each build

# Will bring down your Wi-Fi, and get restored in clean up
sudo ./scripts/bring-up-real-iface.sh

./scripts/build-and-run.sh -p -b native_sim -- -DEXTRA_DTC_OVERLAY_FILE=overlays/real-card.overlay -DEXTRA_CONF_FILE=overlays/wifi.conf

# Should restore your Wi-Fi
sudo ./scripts/clean-up-real-iface.sh
```

Both Wi-Fi builds pass `overlays/wifi.conf` — the overlay picks the interface, the
conf turns the networking stack on. They are always passed together.
