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
  FILES:=$(PKG_BUILD_DIR)/{fbtft.ko,fb_ili9341.ko,fb.ko,fb_sys_fops.ko,syscopyarea.ko,sysfillrect.ko,sysimgblt.ko,fb_defio.ko,fbtft_device.ko}
  KCONFIG:=
endef

define KernelPackage/fbtft/description
  Linux Framebuffer drivers for small TFT LCD display modules
endef

EXTRA_KCONFIG:= \
    CONFIG_FB_DEFERRED_IO=y \
    CONFIG_FB_SYS_FILLRECT=m \
    CONFIG_FB_SYS_COPYAREA=m \
    CONFIG_FB_SYS_IMAGEBLIT=m \
    CONFIG_FB_SYS_FOPS=m \
    CONFIG_FB_TFT_FBTFT_DEVICE=m \
    CONFIG_FB_TFT=m \
    CONFIG_FB_TFT_ILI9341=m

EXTRA_CFLAGS:= \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
    $(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS1:= \
    ARCH="$(LINUX_KARCH)" \
    CROSS_COMPILE="$(TARGET_CROSS)" \
    SUBDIRS="$(LINUX_DIR)/drivers/video" \
    $(EXTRA_KCONFIG)

MAKE_OPTS2:= \
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
	$(RM) $(LINUX_DIR)/drivers/video/*.o
	$(RM) $(LINUX_DIR)/drivers/video/*.ko
	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS1) modules
	cp $(LINUX_DIR)/drivers/video/*.o $(PKG_BUILD_DIR)/
	cp $(LINUX_DIR)/drivers/video/*.ko $(PKG_BUILD_DIR)/
	$(MAKE) -C "$(LINUX_DIR)" $(MAKE_OPTS2) modules
endef

$(eval $(call KernelPackage,fbtft))
