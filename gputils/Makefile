#
# Copyright (C) 2010-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gputils
PKG_VERSION:=1.5.0
PKG_SOURCE_URL:=@SF/gputils
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-${PKG_RELEASE}.tar.gz
PKG_MD5SUM:=60d4da7cd026a4e611120ed51b6da35c

include $(INCLUDE_DIR)/package.mk

define Package/gputils
  SECTION:=development
  CATEGORY:=Development
  TITLE:=Utils for Microchip PIC microcontrollers
  URL:=http://gputils.sourceforge.net/
  DEPENDS:=
endef

define Package/gputils/description
  GPUTILS is a collection of tools for the Microchip (TM) PIC microcontrollers. It includes
  gpasm, gplink, and gplib. It's distributed under the terms of the GNU Public License.
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
	 CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -DGPUTILS_SVN_VERSION=1285"
endef

define Package/gputils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpasm/gpasm $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f25k80.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f26k80.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f45k80.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f46k80.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f4580.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f4680.inc $(1)/usr/share/gputils/header
endef

$(eval $(call BuildPackage,gputils))
