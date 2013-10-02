#
# Copyright (C) 2013 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gpio-proxyd
PKG_VERSION:=0.01
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/gpio-proxyd
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=gpio-proxyd daemon
  DEPENDS:=+kmod-gpio-proxy
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/gpio-proxyd/description
  Allows serving of gpio-proxy requests from remote clients
  Written by Biffereos
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

define Package/gpio-proxyd/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpio-proxyd $(1)/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/gpio-proxyd.init $(1)/etc/init.d/gpio-proxyd
endef

$(eval $(call BuildPackage,gpio-proxyd))
