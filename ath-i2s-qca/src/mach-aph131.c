/*
 * Atheros APH131 reference board support
 *
 * Copyright (c) 2013 Qualcomm Atheros
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <linux/platform_device.h>
#include <linux/ar8216_platform.h>

#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "pci.h"
#include "dev-ap9x-pci.h"
#include "dev-gpio-buttons.h"
#include "dev-eth.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-nand.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define APH131_GPIO_LED_USB		4
#define APH131_GPIO_LED_WLAN_5G		12
#define APH131_GPIO_LED_WLAN_2G		13
#define APH131_GPIO_LED_STATUS_RED	14
#define APH131_GPIO_LED_WPS_RED		15
#define APH131_GPIO_LED_STATUS_GREEN	19
#define APH131_GPIO_LED_WPS_GREEN	20

#define APH131_GPIO_BTN_WPS		16
#define APH131_GPIO_BTN_RFKILL		21

#define APH131_KEYS_POLL_INTERVAL	20	/* msecs */
#define APH131_KEYS_DEBOUNCE_INTERVAL	(3 * APH131_KEYS_POLL_INTERVAL)

#define APH131_MAC0_OFFSET		0
#define APH131_MAC1_OFFSET		6
#define APH131_WMAC_CALDATA_OFFSET	0x1000

static struct gpio_led aph131_leds_gpio[] __initdata = {
	{
		.name		= "aph131:green:status",
		.gpio		= APH131_GPIO_LED_STATUS_GREEN,
		.active_low	= 1,
	},
	{
		.name		= "aph131:red:status",
		.gpio		= APH131_GPIO_LED_STATUS_RED,
		.active_low	= 1,
	},
	{
		.name		= "aph131:green:wps",
		.gpio		= APH131_GPIO_LED_WPS_GREEN,
		.active_low	= 1,
	},
	{
		.name		= "aph131:red:wps",
		.gpio		= APH131_GPIO_LED_WPS_RED,
		.active_low	= 1,
	},
	{
		.name		= "aph131:red:wlan-2g",
		.gpio		= APH131_GPIO_LED_WLAN_2G,
		.active_low	= 1,
	},
	{
		.name		= "aph131:red:usb",
		.gpio		= APH131_GPIO_LED_USB,
		.active_low	= 1,
	}
};

static struct gpio_keys_button aph131_gpio_keys[] __initdata = {
	{
		.desc		= "WPS button",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = APH131_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= APH131_GPIO_BTN_WPS,
		.active_low	= 1,
	},
	{
		.desc		= "RFKILL button",
		.type		= EV_KEY,
		.code		= KEY_RFKILL,
		.debounce_interval = APH131_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= APH131_GPIO_BTN_RFKILL,
		.active_low	= 1,
	},
};

static struct mdio_board_info aph131_mdio0_info[] = {
	{
		.bus_id = "ag71xx-mdio.1",
		.phy_addr = 5,
	},
};

static void __init aph131_gmac_setup(void)
{
	void __iomem *base;
	u32 t;
	base = ioremap(QCA955X_GMAC_BASE, QCA955X_GMAC_SIZE);

	t = __raw_readl(base + QCA955X_GMAC_REG_ETH_CFG);

	t &= ~(QCA955X_ETH_CFG_RGMII_GMAC0 | QCA955X_ETH_CFG_SGMII_GMAC0);
	/* clear bit 6, then GMAC0 is MII, and GMAC1 is SGMII */
	t |= QCA955X_ETH_CFG_RGMII_GMAC0;

	__raw_writel(t, base + QCA955X_GMAC_REG_ETH_CFG);

	iounmap(base);
}

static void __init aph131_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	ath79_register_m25p80(NULL);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(aph131_leds_gpio),
				 aph131_leds_gpio);
	ath79_register_gpio_keys_polled(-1, APH131_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(aph131_gpio_keys),
					aph131_gpio_keys);

	ath79_register_usb();
	ath79_register_wmac(art + APH131_WMAC_CALDATA_OFFSET, NULL);
	ath79_register_pci();

	aph131_gmac_setup();

	ath79_register_mdio(1, 0x0);

	ath79_init_mac(ath79_eth0_data.mac_addr, art + APH131_MAC0_OFFSET, 0);

	mdiobus_register_board_info(aph131_mdio0_info,
				    ARRAY_SIZE(aph131_mdio0_info));

	/* GMAC0 is connected to PLC mac */
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
	ath79_eth0_data.speed = SPEED_100;
	ath79_eth0_data.duplex = DUPLEX_FULL;
	ath79_register_eth(0);

	ath79_init_mac(ath79_eth1_data.mac_addr, art + APH131_MAC1_OFFSET, 0);
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_SGMII;
	ath79_eth1_data.phy_mask = BIT(5);
	ath79_eth1_data.mii_bus_dev = &ath79_mdio1_device.dev;
	ath79_eth1_pll_data.pll_1000 = 0x06000000;
	ath79_register_eth(1);
}

MIPS_MACHINE(ATH79_MACH_APH131, "APH131", "Atheros APH131 reference board",
	     aph131_setup);
