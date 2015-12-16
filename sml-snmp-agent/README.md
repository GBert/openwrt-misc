EDL21 Power Meter Interface
===========================

This repo contains a software project for a EDL21 power meter interface with SNMP support

[!["Power Meter 1"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/IMG_7573_s.JPG)](https://raw.githubusercontent.com/GBert/openwrt-misc/master/sml-snmp-agent/pictures/IMG_7573.JPG)
[!["Power Meter 2"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/IMG_7578_s.JPG)](https://raw.githubusercontent.com/GBert/openwrt-misc/master/sml-snmp-agent/pictures/IMG_7578.JPG)

My setup is working with a HAME A5 which is quite similar to https://wiki.openwrt.org/toh/unbranded/a5-v11 

The router is connected with a sub 2 Euro USB2Serial board and a simple adapter to the power meter:

!["Adapter"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/adapter.png)

The software is a simple SNMPv1-Agent with EDL21 support. I've created a Cacti template and use the setup as a data source:

!["Cacti"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/cacti.png)

### Status

- working, but needs to be daemonized with nohup - will be fixed some day

### Notice

The A5-V11 (available for less than 7 Euro) seems to be the better choice as of today: it has 32MByte RAM instead of 16Mbyte.

