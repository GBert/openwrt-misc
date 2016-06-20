#
# Copyright (C) 2016 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sunxi_tools
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/sunxi_tools
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=SUNXI CAN IP Dump
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/sunxi_tools/description
  SUNXI Tools (CAN IP Dumper)
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

define Package/sunxi_tools/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sunxi_candumper $(1)/usr/bin/
endef

$(eval $(call BuildPackage,sunxi_tools))
