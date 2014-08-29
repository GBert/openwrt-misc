#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=fbtft
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/fbtft
  SUBMENU:=Video Support
  TITLE:=Linux Framebuffer driver
  DEPENDS:=@GPIO_SUPPORT
  FILES:=$(PKG_BUILD_DIR)/{fbtft.ko,fb_ili9341.ko} $(LINUX_DIR)/drivers/video/fb.ko
  KCONFIG:=
endef

define KernelPackage/fbtft/description
  inux Framebuffer drivers for small TFT LCD display modules
endef

EXTRA_KCONFIG:= \
    CONFIG_FB_SYS_FILLRECT=y \
    CONFIG_FB_SYS_COPYAREA=y \
    CONFIG_FB_SYS_IMAGEBLIT=y \
    CONFIG_FB_SYS_FOPS=y \
    CONFIG_FB_DEFERRED_IO=y \
    CONFIG_FB_TFT=m \
    CONFIG_FB_TFT_ILI9341=m \
    CONFIG_FB_DEFERRED_IO=y

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

$(eval $(call KernelPackage,fbtft))
