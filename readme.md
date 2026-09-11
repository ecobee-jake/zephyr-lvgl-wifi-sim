## pre-req:
`sudo dpkg --add-architecture i386 && sudo apt update`

`sudo apt install libnl-3-dev:i386 libnl-genl-3-dev:i386 gcc-multilib pkg-config hostapd`

`export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig`


## Run zephyr sim with *simulated* Linux network interface.
What it does: Zephyr sim can scan for a simulated AP, STA connect, and ping upstream IP
```
sudo ./scripts/bring-up-dev-iface.sh

./scripts/build-and-run.sh -p -b native_sim

sudo ./scripts/clean-up-dev-iface.sh
```

## Run zephyr sim with real Linux network interface.
What it does: Zephyr sim can scan for real APs, but is not able to connect to upstream

```
# Will bring down your Wi-Fi, and get restored in clean up
sudo ./scripts/bring-up-real-iface.sh

./scripts/build-and-run.sh -p -b native_sim -- -DEXTRA_DTC_OVERLAY_FILE=overlays/real-card.overlay # Run the zephyr simulator firmware to use real linux iface

# Should restore your Wi-Fi
sudo ./scripts/clean-up-real-iface.sh
```