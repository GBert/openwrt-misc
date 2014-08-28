#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=i2c-ralink-test
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/i2c-ralink-test
  SUBMENU:=I2C support
  TITLE:=I2C GPIO Test
  DEPENDS:=@GPIO_SUPPORT
  DEPENDS:=kmod-i2c-core
  FILES:=$(PKG_BUILD_DIR)/i2c-ralink.ko
  KCONFIG:=
endef

define KernelPackage/i2c-ralink-test/description
 Kernel module Ralink I2C testing
endef

EXTRA_KCONFIG:= \
    CONFIG_I2C_RALINK_TEST=m \

EXTRA_CFLAGS:= \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
    ARCH="$(LINUX_KARCH)" \
    CROSS_COMPILE="$(TARGET_CROSS)" \
    SUBDIRS="$(PKG_BUILD_DIR)" \
    EXTRA_CFLAGS="$(EXTRA_CFLAGS) -DDEBUG=1" \
    $(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS) modules
endef

$(eval $(call KernelPackage,i2c-ralink-test))
