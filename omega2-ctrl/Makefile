# Copyright (C) 2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=omega2-ctrl
PKG_RELEASE:=0.1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/omega2-ctrl
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Omega2 GPIO tool
endef

define Package/omega2-ctrl/description
  Omega2 pin multiplexer tool
endef

define Build/Prepare
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -std=gnu99"
endef

define Package/omega2-ctrl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/omega2-ctrl $(1)/usr/bin/
endef

$(eval $(call BuildPackage,omega2-ctrl))
