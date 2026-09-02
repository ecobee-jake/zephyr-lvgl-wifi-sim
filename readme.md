

## Build
`west build -p always -b native_sim/native/64`

## Run
`west build -t run`

## pre-req:
`sudo dpkg --add-architecture i386 && sudo apt update`

`sudo apt install libnl-3-dev:i386 libnl-genl-3-dev:i386 gcc-multilib pkg-config hostapd`

`export PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig`


zephyr was here
jake@jake-ThinkPad-P14s-Gen-4:~/firmware/zephyr-test/zephyr$ git status
HEAD detached at refs/heads/manifest-rev
nothing to commit, working tree clean



new build command:
PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig west build -p always -b native_sim
