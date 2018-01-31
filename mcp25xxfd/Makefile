#
# Copyright (C) 2018 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mcp25xxfd
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/mcp25xxfd
  SUBMENU:=CAN Support
  TITLE:=MCP25XXFD CAN Support
  DEPENDS:=@GPIO_SUPPORT +kmod-can
  FILES:=$(PKG_BUILD_DIR)/mcp25xxfd.ko
  KCONFIG:=
endef

define KernelPackage/mcp25xxfd/description
  This module adds support for the Microchip mcp2517fd CAN-FD controller.
  The mcp2517fd is capable of transmitting and receiving standard data
  frames, extended data frames, remote frames and Can-FD frames.
  The mcp2517fd interfaces with the host over SPI.
endef

EXTRA_KCONFIG:= \
    CONFIG_MCP25XXFD=m \

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

$(eval $(call KernelPackage,mcp25xxfd))
