EDL21 power meter Interface
===========================

This repo contains a project for a EDL21 power meter interface with SNMP support

!["Power Meter"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/libehz-leser.jpg)

Only a sub 2$ USB2Serail board and a simple adapter is needed:

!["Adapter"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/adapter.png)

and some software for building a SNMP-Agent. This will enable to feed Cacti e.g.:

!["Cacti"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/cacti.png)

My setup is working with a HAME A5 which is quite similar to https://wiki.openwrt.org/toh/unbranded/a5-v11
The A5-V11 (available for less than 7 Euro) seems to be the better choice as of today: it has 32MByte RAM instead of 16Mbyte
### Status

- working, but needs to daemonize externally

