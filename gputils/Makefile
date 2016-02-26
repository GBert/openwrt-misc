#
# Copyright (C) 2010-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gputils
PKG_VERSION:=1.4.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-${PKG_RELEASE}.tar.gz
PKG_MD5SUM:=e0d074b9d481b2419b0411938d046102

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
	 CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)"
endef

define Package/gputils/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpasm/gpasm $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/root/header
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f25k80.inc $(1)/root/header/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f26k80.inc $(1)/root/header/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f45k80.inc $(1)/root/header/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f46k80.inc $(1)/root/header/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f4580.inc $(1)/root/header/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/header/p18f4680.inc $(1)/root/header/
endef

$(eval $(call BuildPackage,gputils))
