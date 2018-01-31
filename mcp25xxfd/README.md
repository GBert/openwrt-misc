MCP2517FD CAN Module
====================

Add following entry
```
src-git openwrtmisc https://github.com/GBert/openwrt-misc
```
to feeds.conf.default if it doesn't exist.

Do
```
scripts/feeds update -a
scripts/feeds install kmod-mcp25xxfd
```

RPi2+3
------
For RPi mount the first partition of the SD-card image and copy the overlays
to the overlay directory. Add following to the config.txt:
```
core_freq=250
overlay_prefix=overlays/
dtoverlay=mcp2517fd-can0
dtparam=interrupt=25
dtparam=oscillator=4000000
dtparam=i2c_arm=on
```

