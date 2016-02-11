#!/bin/sh

insmod c2tool-gpio c2ck=268 c2d=270
mknod /dev/c2tool-gpio c 180 0
chmod 666 /dev/c2tool-gpio
