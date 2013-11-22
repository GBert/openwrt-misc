#
# Copyright (C) 2013 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=eepro100-diag
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/eepro100-diag
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:= Diagnostic and setup program for the Intel EEPro100
  AUTHOR:=Donald Becker
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/eepro100-diag/description
  This is a diagnostic and EEPROM setup program for the Ethernet adapters
  based on the Intel "Speedo3" chip series: the i82557, '558 and '559.
  These chips are used on the EtherExpress Pro100B, and EEPro PCI 10+.
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

define Package/eepro100-diag/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/eepro100-diag $(1)/usr/bin/
endef

$(eval $(call BuildPackage,eepro100-diag))
