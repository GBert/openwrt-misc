#
# Copyright (C) 2015 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=c2tool
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/c2tool
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libbfd
  TITLE:= Silabs C2 tool
  MAINTAINER:=Gerhard Bertelsmann
endef

define Package/c2tool/description
  Programms Silabs C8051 family with C2 interface via 3 GPIOs
  needs a monoflobe (short low puls)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	make -C $(PKG_BUILD_DIR)		\
		$(TARGET_CONFIGURE_OPTS)	\
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS)"	\
		LIBS="$(TARGET_LDFLAGS) -llibbfd"
endef

define Package/c2tool/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/c2tool $(1)/usr/bin/
endef

$(eval $(call BuildPackage,c2tool))
