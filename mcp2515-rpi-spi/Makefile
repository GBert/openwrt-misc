#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mcp2515-rpi-spi
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/mcp2515-rpi-spi
  SUBMENU:=CAN Support
  TITLE:=MCP2515 banged module
  DEPENDS:=@GPIO_SUPPORT +kmod-can
  FILES:=$(PKG_BUILD_DIR)/mcp2515-rpi-spi.ko
  KCONFIG:=
endef

define KernelPackage/mcp2515-rpi-spi/description
  This module combines the CAN MCP2515 and RPI-SPI code. Without all the Kernel
  overhead from the SPI modules this should be less sensitive to packet loss.
  This is bad code will never be mainline though ...
endef

EXTRA_KCONFIG:= \
    CONFIG_MCP2515_RPI_SPI=m \

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

$(eval $(call KernelPackage,mcp2515-rpi-spi))
