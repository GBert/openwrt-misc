Modded U-Boot 1.1.4 for WR841N V8 from pepe2k 
==========

compiled with gcc version 4.8.3 (OpenWrt/Linaro GCC 4.8-2014.04 r44461)

```
export PATH=$PATH:~/projekte/openwrt/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin

diff --git a/Makefile b/Makefile
index 755eb00..bad0ad3 100755
--- a/Makefile
+++ b/Makefile
@@ -1,7 +1,7 @@
 export BUILD_TOPDIR=$(PWD)
 export STAGING_DIR=$(BUILD_TOPDIR)/tmp

-export MAKECMD=make --silent --no-print-directory ARCH=mips CROSS_COMPILE=mips-linux-gnu-
+export MAKECMD=make --silent --no-print-directory ARCH=mips CROSS_COMPILE=mips-openwrt-linux-uclibc-

 # boot delay (time to autostart boot command)
 export CONFIG_BOOTDELAY=1
```

```
    Image Name: u-boot image
       Created: Wed Apr 22 12:51:40 2015
    Image Type: MIPS Linux Firmware (lzma compressed)
     Data Size: 45840 Bytes = 44.77 kB = 0.04 MB
  Load Address: 0x80010000
   Entry Point: 0x80010000
```

