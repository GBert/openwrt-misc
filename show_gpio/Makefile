#
# Copyright (C) 2013 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=show_gpio
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/show_gpio
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:= Show GPIO pin aasignment on TL-WR841N(D)
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/show_gpio/description
  The AR9341 SoC is able to mux different functions onto the GPIOs.
  This utility shows the PIN assigments on TL-WR841N(D)
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

define Package/show_gpio/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/show-gpio $(1)/usr/bin/
endef

$(eval $(call BuildPackage,show_gpio))
