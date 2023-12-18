#
# Copyright (C) 2010-2018 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gputils
PKG_REV:=1325
PKG_VERSION:=1.5.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://sourceforge.net/projects/gputils/files/gputils/1.5.0/
PKG_HASH:=8fb8820b31d7c1f7c776141ccb3c4f06f40af915da6374128d752d1eee3addf2
# PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=GPLv3
PKG_LICENSE_FILES:=COPYING

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
	 CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -DGPUTILS_SVN_VERSION=$(PKG_REV)"
endef

define Package/gputils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpasm/gpasm $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p16f15325.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p16f1705.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p16f1709.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f??k8?.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f??80.inc $(1)/usr/share/gputils/header
	$(INSTALL_DIR) $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lkr/16f15325_g.lkr $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lkr/16f1705_g.lkr $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lkr/16f1709_g.lkr $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lkr/18f??k8?_g.lkr $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lkr/18f??80_g.lkr $(1)/usr/share/gputils/lkr
endef

$(eval $(call BuildPackage,gputils))
