#
# Copyright (C) 2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gpio-test
PKG_RELEASE:=1
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/gpio-test
  SUBMENU:=Other modules
  TITLE:=Simple GPIO Test
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/{gpio-test.ko,gpio-irq-test.ko,gpio-toggle-test.ko,gpio-hw-irq-latency-test.ko,gpio-irq-latency-test.ko}
  KCONFIG:=
endef

define KernelPackage/gpio-test/description
 Simple GPIO test suite including 'toggle speed test' and 'GPIO IRQ latency test'
endef

EXTRA_KCONFIG:= \
    CONFIG_GPIO_TEST=m

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

$(eval $(call KernelPackage,gpio-test))
