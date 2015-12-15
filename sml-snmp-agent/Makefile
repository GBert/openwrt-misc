#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sml_snmp_agent
PKG_VERSION:=1.0

PKG_SOURCE_MIRROR:=0 
PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)/
PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:= SNMP Agent for EDL21 power meter
  URL:=http://lnxpps.de/openwrt/hame-mpra5/
  DEPENDS:=+libpthread
endef

define Build/Prepare
        $(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(MAKE)	-C $(PKG_BUILD_DIR)/sml $(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -I$(PKG_BUILD_DIR)/sml/include -fPIC -D_NO_UUID_LIB"
	$(MAKE)	-C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -I$(PKG_BUILD_DIR)/sml/include -L$(PKG_BUILD_DIR)/sml/lib"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sml_server $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/sml_server.init $(1)/etc/init.d/sml_server
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
