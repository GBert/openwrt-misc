GPIO proxy module
=================

programmed by bifferos

GPIO proxy kernel module to manipulate GPIOs accross networks 
Please have a look at the daemon part.

further informations: https://forum.openwrt.org/viewtopic.php?id=15739

howto use:

add following line to your feeds.conf.default:

`src-git openwrtmisc https://github.com/GBert/openwrt-misc.git`

and do

`scripts/feeds update -a`

`scripts/feeds install kmod-gpio-proxy`

`scripts/feeds install gpio-proxyd`

`make menuconfig `

mark gpio-proxyd under 'Utilities' and compile with

`make package/gpio-proxyd/{clean,compile} V=s`

`make package/gpio-proxy-module/{clean,compile} V=s`


copy 'bin/\<platform\>/kmod-gpio-proxy\*' and 'bin/\<platform\>/gpio-proxyd\*' to your OpenWrt router and install it

