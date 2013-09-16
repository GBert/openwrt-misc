#
# Copyright (C) 2013 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=show_audio_pll
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/show_audio_pll
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=show Audio PLL settings on AR934X
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/show_audio_pll/description
  The AR9341 SoC is able to use different I2S audio settings
  This utility shows the master and clock settings
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	make -C $(PKG_BUILD_DIR)		\
		$(TARGET_CONFIGURE_OPTS)	\
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)"	\
		LIBS="$(TARGET_LDFLAGS)"
endef

define Package/show_audio_pll/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/show-audio-pll $(1)/usr/bin/
endef

$(eval $(call BuildPackage,show_audio_pll))
