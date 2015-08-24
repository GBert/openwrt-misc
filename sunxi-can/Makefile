#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=sun7i-can
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/sun7i-can
  SUBMENU:=CAN Support
  TITLE:=SUNXI CAN-Bus
  DEPENDS:=+kmod-can
  FILES:=$(PKG_BUILD_DIR)/sun7i-can.ko
  KCONFIG:=
endef

define KernelPackage/sun7i-can/description
  This module is tesing code for the SUNXI CAN-Bus module
  Target: suing generic OF sja1000 kernel module
endef

EXTRA_KCONFIG:= \
    CONFIG_SUN7I_CAN=m \

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
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS) modules
endef

$(eval $(call KernelPackage,sun7i-can))
