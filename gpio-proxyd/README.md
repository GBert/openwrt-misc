GPIO proxyd
===========

programmed by bifferos

GPIO proxy daemon to manipulate GPIOs accross networks

further informations: https://forum.openwrt.org/viewtopic.php?id=15739

howto install
-------------

add following line to your feeds.conf.default:

`src-git openwrtmisc https://github.com/GBert/openwrt-misc.git`

and do

<pre><code>scripts/feeds update -a
scripts/feeds install kmod-gpio-proxy
scripts/feeds install gpio-proxyd
make menuconfig</pre></code>

mark gpio-proxyd under 'Utilities' and compile with

<pre><code>make package/gpio-proxyd/{clean,compile} V=s
make package/gpio-proxy-module/{clean,compile} V=s</pre></code>


copy `bin/<platform>/kmod-gpio-proxy*` and `bin/<platform>/gpio-proxyd*` to your OpenWrt router and install it

howto use
---------

please have a look in examples/ and the URL above


