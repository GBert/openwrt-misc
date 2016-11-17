#
# Copyright (C) 2016 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=dtoverlay
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/dtoverlay
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Device Tree Overlay Tools
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/dtoverlay/description
  Device Tree Overlay Tools
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	make -C $(PKG_BUILD_DIR)		\
		$(TARGET_CONFIGURE_OPTS)	\
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)"	\
		LIBS="$(TARGET_LDFLAGS) -I."
endef

define Package/dtoverlay/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dtoverlay $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/dtmerge $(1)/usr/bin/
endef

$(eval $(call BuildPackage,dtoverlay))
