#
# Copyright (C) 2015 OpenWrt
# See LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=wiringbp-pi
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/wiringbp-pi
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=WiringBP
  MAINTAINER:=Gerhard Bertelsmann
  DEPENDS:=
endef

define Package/wiringbp-pi/description
  This is a modified WiringPi for Banana Pi
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	make -C $(PKG_BUILD_DIR)/wiringPi	\
		$(TARGET_CONFIGURE_OPTS)	\
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -I/$(PKG_BUILD_DIR)/wiringPi/ -I/$(PKG_BUILD_DIR)/devLib/"	\
		LIBS="$(TARGET_LDFLAGS)"
	ln -sf $(PKG_BUILD_DIR)/wiringPi/libwiringPi.so.2.0 $(PKG_BUILD_DIR)/wiringPi/libwiringPi.so
	make -C $(PKG_BUILD_DIR)/devLib		\
		$(TARGET_CONFIGURE_OPTS)	\
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -I/$(PKG_BUILD_DIR)/wiringPi/ -I/$(PKG_BUILD_DIR)/devLib/"	\
		LIBS="$(TARGET_LDFLAGS)"
	ln -sf $(PKG_BUILD_DIR)/devLib/libwiringPiDev.so.2.0 $(PKG_BUILD_DIR)/devLib/libwiringPiDev.so
	make -C $(PKG_BUILD_DIR)/gpio		\
		$(TARGET_CONFIGURE_OPTS)	\
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -I/$(PKG_BUILD_DIR)/wiringPi/ -I/$(PKG_BUILD_DIR)/devLib/"	\
		LIBS="$(TARGET_LDFLAGS) -L$(PKG_BUILD_DIR)/devLib -L$(PKG_BUILD_DIR)/wiringPi -lwiringPi -lwiringPiDev -lpthread -lm"
	make -C $(PKG_BUILD_DIR)/examples speed	\
		$(TARGET_CONFIGURE_OPTS)	\
		CFLAGS="$(TARGET_CFLAGS) $(TARGET_CPPFLAGS) -I/$(PKG_BUILD_DIR)/wiringPi/ -I/$(PKG_BUILD_DIR)/devLib/"	\
		LIBS="$(TARGET_LDFLAGS) -L$(PKG_BUILD_DIR)/devLib -L$(PKG_BUILD_DIR)/wiringPi -lwiringPi -lwiringPiDev -lm"
endef

define Package/wiringbp-pi/install
	$(INSTALL_DIR) $(1)/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wiringPi/libwiringPi.so.2.0 $(1)/lib
	ln -sf libwiringPi.so.2.0     $(1)/lib/libwiringPi.so
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/devLib/libwiringPiDev.so.2.0 $(1)/lib
	ln -sf libwiringPiDev.so.2.0  $(1)/lib/libwiringPiDev.so
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpio/gpio $(1)/usr/bin/gpio
endef

$(eval $(call BuildPackage,wiringbp-pi))
