#!/bin/sh

insmod fb_defio
insmod fb
insmod fbtft
insmod fb_ili9341
insmod fbtft_device name=tm022hdh26 busnum=32766 gpios=reset:11,led:12,dc:13 rotate=270
