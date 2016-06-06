#
# Copyright (C) 2016 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=renesas-flashtool
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/renesas-flashtool
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libftdi1
  TITLE:= Renesas Flash Tool
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/renesas-flashtool/description
	Renesas Flash Tool by dj delorie
endef

TARGET_CFLAGS+= -I$(STAGING_DIR)/usr/include/libftdi1 \

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	make -C $(PKG_BUILD_DIR)		\
		$(TARGET_CONFIGURE_OPTS)	\
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)"	\
		LIBS="$(TARGET_LDFLAGS) -lftdi1"
endef

define Package/renesas-flashtool/install
	$(INSTALL_DIR) $(1)/usr/bin
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/uflash $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sflash $(1)/usr/bin/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rx $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rxs $(1)/usr/bin/
#	$(INSTALL_BIN) $(PKG_BUILD_DIR)/eeprom $(1)/usr/bin/
endef

$(eval $(call BuildPackage,renesas-flashtool))
