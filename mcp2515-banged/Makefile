#
# Copyright (C) 2022 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mcp2515-banged
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
  SUBMENU:=CAN Support
  TITLE:=MCP2515 banged module
  DEPENDS:=@GPIO_SUPPORT +kmod-can
  FILES:=$(PKG_BUILD_DIR)/mcp2515-banged.ko
  KCONFIG:=
endef

define KernelPackage/mcp2515-banged/description
  This module combines the CAN MCP2515 und SPI bit-banged module to test how well
  this could work with all the Kernel overhead from the SPI modules wiped out.
  This is bad code: it locks the CPU for tens of microseconds - don't do it this way ...
endef

EXTRA_KCONFIG:= \
    CONFIG_MCP2515_BANGED=m \

EXTRA_CFLAGS:= \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

ifneq (,$(findstring mt76x8,$(CONFIG_TARGET_SUBTARGET)))
        EXTRA_CFLAGS+= -DMT7688
endif

ifneq (,$(findstring mt7688,$(CONFIG_TARGET_SUBTARGET)))
        EXTRA_CFLAGS+= -DMT7688
endif

ifneq (,$(findstring rt305x,$(CONFIG_TARGET_SUBTARGET)))
        EXTRA_CFLAGS+= -DRT305X
endif

MAKE_OPTS:= \
        $(KERNEL_MAKE_FLAGS) \
        M="$(PKG_BUILD_DIR)" \
        EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
        $(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS) modules
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
