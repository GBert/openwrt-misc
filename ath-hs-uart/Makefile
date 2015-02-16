#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ath-hs-uart
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ath-hs-uart
  SUBMENU:=Other modules
  TITLE:=Atheros 934X High Speed Uart
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/ar934x_hs_uart.ko
  KCONFIG:=
  AUTOLOAD:=$(call AutoProbe,ar934x_hs_uart)
endef

define KernelPackage/ath-hs-uart/description
 Atheros AR934x High Speed Uart module
endef

EXTRA_KCONFIG:= \
    CONFIG_ATH934X_HS_UART=m

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
	$(MAKE) -C "$(LINUX_DIR)" \
	$(MAKE_OPTS) \
	modules
endef

$(eval $(call KernelPackage,ath-hs-uart))
