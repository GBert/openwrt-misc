#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mcp2515-usb
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/mcp2515-usb
  SUBMENU:=CAN Support
  TITLE:=MCP2515 CAN Bus Monitor Demo Board driver
  DEPENDS:=@GPIO_SUPPORT +kmod-usb-core +kmod-can +kmod-can-dev
  FILES:=$(PKG_BUILD_DIR)/mcp2515-usb.ko
  KCONFIG:=
endef

define KernelPackage/mcp2515-usb/description
  This module is the SocketCAN driver for Microchips MCP2515 CAN Bus Monitor Demo Board
  aka MCP2515DM-BM
endef

EXTRA_KCONFIG:= \
    CONFIG_MCP2515_USB=m \

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

$(eval $(call KernelPackage,mcp2515-usb))
