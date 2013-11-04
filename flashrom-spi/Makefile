#
# Copyright (C) 2010-2013 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=flashrom
PKG_VERSION:=0.9.7
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://download.flashrom.org/releases
PKG_MD5SUM:=d239bafa1b46619d851f70f84a5b122a

include $(INCLUDE_DIR)/package.mk

define Package/flashrom-spi
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=FlashROM Utility - SPI only
  URL:=http://www.flashrom.org/
  DEPENDS:=
endef

define Package/flashrom-spi/description
 flashrom is a utility for identifying, reading, writing, verifying
 and erasing flash chips. Stripped down version for SPI only.
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(TARGET_CONFIGURE_OPTS) \
	 CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)" \
	 CONFIG_DEFAULT_PROGRAMMER=CONFIG_LINUX_SPI \
	 CONFIG_INTERNAL=0 NEED_PCI=0 \
	 CONFIG_NIC3COM=0 CONFIG_GFXNVIDIA=0 CONFIG_SATASII=0 CONFIG_DRKAISER=0 \
	 CONFIG_NICREALTEK=0 CONFIG_NICINTEL=0 CONFIG_NICINTEL_SPI=0 \
	 CONFIG_OGP_SPI=0 CONFIG_SATAMV=0 CONFIG_RAYER_SPI=0 CONFIG_BUSPIRATE_SPI=0 \
	 CONFIG_SERPROG=0 CONFIG_PONY_SPI=0
endef

define Package/flashrom-spi/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/flashrom $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,flashrom-spi))
