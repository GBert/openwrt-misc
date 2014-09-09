#
# Copyright (C) 2013 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=framebuffer_llp
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/framebuffer_llp
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Low Level Framebuffer Tests 
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/framebuffer_llp/description
  low level framebuffer programming
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

define Package/framebuffer_llp/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fbtest_01 $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fbtest_02 $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fbtest_03 $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fbtest_04 $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fbtest_05 $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fbtest_06 $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fbtest_08 $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fbfire $(1)/usr/bin/
endef

$(eval $(call BuildPackage,framebuffer_llp))
