#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=c2port-gpio
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/c2port-gpio
  SUBMENU:=Other modules
  TITLE:=C2PORT GPIO
  DEPENDS:=@GPIO_SUPPORT
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/{c2port-core.ko,c2port-gpio.ko}
  KCONFIG:=
endef

define KernelPackage/c2port-gpio/description
 Kernel module C2PORT GPIO
endef

EXTRA_KCONFIG:= \
    CONFIG_C2PORT_GPIO=m \
    CONFIG_C2PORT_CORE=m

EXTRA_CFLAGS:= \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
    ARCH="$(LINUX_KARCH)" \
    CROSS_COMPILE="$(TARGET_CROSS)" \
    SUBDIRS="$(PKG_BUILD_DIR)" \
    EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
    $(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
#	$(CP) $(LINUX_DIR)/drivers/misc/c2port/core.c $(PKG_BUILD_DIR)/c2port-core.c
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS) modules
	$(MAKE) -C $(PKG_BUILD_DIR)    \
	$(TARGET_CONFIGURE_OPTS)        \
	CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)"    \
	LIBS="$(TARGET_LDFLAGS)" hex2image
endef

$(eval $(call KernelPackage,c2port-gpio))
