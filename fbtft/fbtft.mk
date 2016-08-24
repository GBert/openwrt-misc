#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

VIDEO_MENU:=Video Support

#
# FB TFT Display
#

define KernelPackage/fb-tft
  SUBMENU:=$(VIDEO_MENU)
  TITLE:=Framebuffer support for small TFT displays
  DEPENDS:=@DISPLAY_SUPPORT
  KCONFIG:= \
	CONFIG_STAGING=y \
	CONFIG_FB_TFT=y \
	CONFIG_FB_TFT_AGM1264K_FL=n \
	CONFIG_FB_TFT_BD663474=n \
	CONFIG_FB_TFT_HX8340BN=n \
	CONFIG_FB_TFT_HX8347D=n \
	CONFIG_FB_TFT_HX8353D=n \
	CONFIG_FB_TFT_HX8357D=n \
	CONFIG_FB_TFT_ILI9163=n \
	CONFIG_FB_TFT_ILI9320=n \
	CONFIG_FB_TFT_ILI9325=n \
	CONFIG_FB_TFT_ILI9340=n \
	CONFIG_FB_TFT_ILI9341=m \
	CONFIG_FB_TFT_ILI9481=n \
	CONFIG_FB_TFT_ILI9486=n \
	CONFIG_FB_TFT_PCD8544=m \
	CONFIG_FB_TFT_RA8875=n \
	CONFIG_FB_TFT_S6D02A1=n \
	CONFIG_FB_TFT_S6D1121=n \
	CONFIG_FB_TFT_SSD1289=n \
	CONFIG_FB_TFT_SSD1306=n \
	CONFIG_FB_TFT_SSD1331=n \
	CONFIG_FB_TFT_SSD1351=n \
	CONFIG_FB_TFT_ST7735R=n \
	CONFIG_FB_TFT_ST7789V=n \
	CONFIG_FB_TFT_TINYLCD=n \
	CONFIG_FB_TFT_TLS8204=n \
	CONFIG_FB_TFT_UC1611=n \
	CONFIG_FB_TFT_UC1701=n \
	CONFIG_FB_TFT_UPD161704=n \
	CONFIG_FB_TFT_WATTEROTT=n \
	CONFIG_FB_FLEX=n \
	CONFIG_FB_TFT_FBTFT_DEVICE=m
  FILES:=\
	$(LINUX_DIR)/drivers/staging/fbtft/fbtft_device.ko \
	$(LINUX_DIR)/drivers/staging/fbtft/fbtft.ko \
	$(LINUX_DIR)/drivers/staging/fbtft/fb_ili9341.ko \
	$(LINUX_DIR)/drivers/staging/fbtft/fb_pcd8544.ko
  AUTOLOAD:=$(call AutoLoad,06,fbtft_device)
endef

define KernelPackage/fb-tft/description
 Kernel support for small TFT LCD display modules
endef

$(eval $(call KernelPackage,fb-tft))
