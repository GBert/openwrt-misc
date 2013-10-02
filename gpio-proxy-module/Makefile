#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpio-proxy
PKG_VERSION:=0.1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/gpio-proxy
  SUBMENU:=Other modules
  TITLE:=Kernel driver for GPIO remote access
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/gpio-proxy.ko
  URL:=http://members.aol.com/bifferos/sweex/
  FILES:=$(PKG_BUILD_DIR)/gpio-proxy.$(LINUX_KMOD_SUFFIX)
  VERSION:=$(LINUX_VERSION)+$(PKG_VERSION)-$(PKG_RELEASE)
  AUTOLOAD:=$(call AutoLoad,80,gpio-proxy)
  KCONFIG:=
endef

define KernelPackage/gpio-proxy/description
  Kernel Module for remote GPIO access
endef

EXTRA_KCONFIG:= \
    CONFIG_GPIO_PROXY=m

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

$(eval $(call KernelPackage,gpio-proxy))
