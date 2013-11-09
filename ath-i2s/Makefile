#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ath-i2s
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ath-i2s
  SUBMENU:=Other modules
  TITLE:=Atheros 934X I2S module
  DEPENDS:=
#  FILES:=$(PKG_BUILD_DIR)/ath_i2s.ko
  FILES:=$(PKG_BUILD_DIR)/bit_stream.ko
  KCONFIG:=
endef

define KernelPackage/ath-i2s/description
 Atheros AR934x I2S module
endef

EXTRA_KCONFIG:= \
    CONFIG_ATH934X_I2S=m

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

$(eval $(call KernelPackage,ath-i2s))
