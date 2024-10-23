EDL21 Power Meter Interface
===========================

This repo contains a software project for a EDL21 power meter interface with SNMP support

[!["Power Meter 1"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/IMG_7573_s.JPG)](https://raw.githubusercontent.com/GBert/openwrt-misc/master/sml-snmp-agent/pictures/IMG_7573.JPG)
[!["Power Meter 2"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/IMG_7578_s.JPG)](https://raw.githubusercontent.com/GBert/openwrt-misc/master/sml-snmp-agent/pictures/IMG_7578.JPG)

My setup is working with a HAME A5 which is quite similar to https://wiki.openwrt.org/toh/unbranded/a5-v11 

The router reads with a cheap USB2Serial board and a simple infrared adapter the EDL21 stream from the power meter:

!["Adapter"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/adapter.png)

The software is a simple SNMPv1-Agent with EDL21 support. I've created a Cacti template and use the setup as a data source:

!["Cacti"](https://github.com/GBert/openwrt-misc/blob/master/sml-snmp-agent/pictures/cacti.png)

### Status

- working but lacking some error handling

### Howto integrate
```
cd <openwrt_dir>
echo 'src-git openwrtmisc https://github.com/GBert/openwrt-misc' >> feeds.conf.default
scripts/feeds update -a
scripts/feeds install sml_snmp_agent
make menuconfig
# mark the sml_snmp_agent under Utilities 
```
#### None OpenWRT environment
```
git clone --depth=1 https://github.com/GBert/openwrt-misc.git
cd openwrt-misc/sml-snmp-agent/src/sml
make
cd ..
make
mv sml_server sml-snmp-agent
./sml-snmp-agent -h

Usage: sml-snmp-agent -p <snmp_port> -i <interface> [f]
   Version 1.1

         -p <port>           SNMP port - default 161
         -i <interface>      serial interface - default /dev/ttyUSB0
         -f                  running in foreground

```
### Usage
Volkszaehler.org private SNMP OID 39241 is used (could be easily adapted - look at src/sml_snmp.c)
```
snmpget -c public -v1 <sml-snmp-agent> 1.3.6.1.4.1.39241.16.7.0 # aktuelle Wirkleistung
snmpget -c public -v1 <sml-snmp-agent> 1.3.6.1.4.1.39241.1.8.0 # Bezug Gesamt
snmpget -c public -v1 <sml-snmp-agent> 1.3.6.1.4.1.39241.1.8.1 # Bezug Haupttarif
snmpget -c public -v1 <sml-snmp-agent> 1.3.6.1.4.1.39241.1.8.2 # Bezug Nebentarif
snmpget -c public -v1 <sml-snmp-agent> 1.3.6.1.4.1.39241.2.8.0 # Belieferung
```
### Notice

The A5-V11 (available for less than 7 Euro) seems to be the better choice as of today: it has 32MByte RAM instead of 16Mbyte.

