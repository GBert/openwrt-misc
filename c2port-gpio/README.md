# Silabs C2 programming interface

This kernel module is able to emulate the Silicon Labs 2-Wire Interface through GPIO bitbanging:

https://www.silabs.com/Support%20Documents/TechnicalDocs/C2spec.pdf

## Status: loads and seems to work

```
root@OpenWrt:~# insmod c2port-core
root@OpenWrt:~# insmod c2port-gpio

root@OpenWrt:~# dmesg | grep c2port
[   80.200000] Silicon Labs C2 port support v. 0.51.0 - (C) 2007 Rodolfo Giometti
[   81.680000] c2port c2port0: C2 port uc added
[   81.680000] c2port c2port0: uc flash has 30 blocks x 512 bytes (15360 bytes total)

root@OpenWrt:~# cat /sys/kernel/debug/gpio | grep c2port
 gpio-0   (c2port data        ) in  hi
 gpio-1   (c2port clock       ) in  lo

root@OpenWrt:~# show-gpio  | grep JP2
   JP2 pin 9 GPIO00 I h GPIO
   JP2 pin 3 GPIO01 I l GPIO
   JP2 pin 5 GPIO02 O l GPIO
   JP2 pin 7 GPIO03 I l GPIO

root@OpenWrt:~# ls /sys/class/c2port/c2port0/
access            flash_block_size  flash_erase       reset             uevent
dev_id            flash_blocks_num  flash_size        rev_id
flash_access      flash_data        name              subsystem

root@OpenWrt:~# echo 1 > /sys/class/c2port/c2port0/access
root@OpenWrt:~# # Silabs C8051F502 connected
root@OpenWrt:~# cat /sys/class/c2port/c2port0/dev_id
28
root@OpenWrt:~# cat /sys/class/c2port/c2port0/rev_id
1
```
Please have a look at 'Documentation/misc-devices/c2port.txt' in the kernel tree
