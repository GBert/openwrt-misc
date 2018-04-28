#
# Copyright (C) 2010-2018 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gputils
PKG_REV:=1312
PKG_VERSION:=1.5.2-$(PKG_REV)
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=svn
PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=svn://svn.code.sf.net/p/gputils/code/trunk
PKG_SOURCE:=$(PKG_SOURCE_SUBDIR).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

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

define Build/Configure
endef

define Build/Compile
	cd $(PKG_BUILD_DIR)/gputils && ./configure --prefix=/usr
	$(MAKE) -C $(PKG_BUILD_DIR)/gputils $(TARGET_CONFIGURE_OPTS) \
	 CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -DGPUTILS_SVN_VERSION=$(PKG_REV)"
endef

define Package/gputils/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/gpasm/gpasm $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/header/p16f15325.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/header/p16f1705.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/header/p16f1709.inc $(1)/usr/share/gputils/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/header/p18f??k8?.inc $(1)/usr/share/gputils/header
	$(INSTALL_DIR) $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/lkr/16f15325_g.lkr $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/lkr/16f1705_g.lkr $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/lkr/16f1709_g.lkr $(1)/usr/share/gputils/lkr
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gputils/lkr/18f??k8?_g.lkr $(1)/usr/share/gputils/lkr
endef

$(eval $(call BuildPackage,gputils))
