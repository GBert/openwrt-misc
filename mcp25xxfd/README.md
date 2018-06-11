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

Using
-----
CAN Bit timing isn't perfect (not mcp25xxxfd problem).
Here the 250kB settings:
```
# setup 250kBaud
#  time quantum 250ns time quanta 16 => 16*250ns = 4us => 250kb
ip link set can0 up type can fd off tq 250 prop-seg 6 phase-seg1 7 phase-seg2 2 sjw 1 berr-reporting off restart-ms 100
ip -s -d link show can0
```
