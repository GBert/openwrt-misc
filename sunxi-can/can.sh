#!/bin/sh
io -4 0x01c2006c
echo "enabling CAN clocking"
io -4 -o 0x01c2006c 0x10
io -4 0x01c2006c

io -4 0x01c20904
echo "set CAN pins"
io -4 -o 0x01c20904 0x00440000
io -4 0x01c20904

modprobe sun7i_can
ip link set can0 type can bitrate 250000
ifconfig can0 up
