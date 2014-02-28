/*
 * Atheros REH132 reference board support
 *
 * Copyright (c) 2013 Qualcomm Atheros
 * Copyright (c) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
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

#include <linux/pci.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/ath9k_platform.h>
#include <linux/ar8216_platform.h>

#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-ap9x-pci.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define REH132_GPIO_LED_USB		11
#define REH132_GPIO_LED_WLAN_5G		12
#define REH132_GPIO_LED_WLAN_2G		13
#define REH132_GPIO_LED_STATUS		14
#define REH132_GPIO_LED_WPS		15

#define REH132_GPIO_BTN_WPS		16

#define REH132_KEYS_POLL_INTERVAL	20	/* msecs */
#define REH132_KEYS_DEBOUNCE_INTERVAL	(3 * REH132_KEYS_POLL_INTERVAL)

#define REH132_MAC0_OFFSET		0
#define REH132_MAC1_OFFSET		6
#define REH132_WMAC_CALDATA_OFFSET	0x1000
#define REH132_PCIE_CALDATA_OFFSET	0x5000

static struct gpio_led reh132_leds_gpio[] __initdata = {
	{
		.name		= "reh132:green:status",
		.gpio		= REH132_GPIO_LED_STATUS,
		.active_low	= 1,
	},
	{
		.name		= "reh132:green:wps",
		.gpio		= REH132_GPIO_LED_WPS,
		.active_low	= 1,
	},
	{
		.name		= "reh132:green:wlan-5g",
		.gpio		= REH132_GPIO_LED_WLAN_5G,
		.active_low	= 1,
	},
	{
		.name		= "reh132:green:wlan-2g",
		.gpio		= REH132_GPIO_LED_WLAN_2G,
		.active_low	= 1,
	},
	{
		.name		= "reh132:green:usb",
		.gpio		= REH132_GPIO_LED_USB,
		.active_low	= 1,
	}
};

static struct gpio_keys_button reh132_gpio_keys[] __initdata = {
	{
		.desc		= "WPS button",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = REH132_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= REH132_GPIO_BTN_WPS,
		.active_low	= 1,
	},
};

static struct ar8327_platform_data reh132_ar8216_data = {
};

static struct mdio_board_info reh132_mdio0_info[] = {
	{
		.bus_id = "ag71xx-mdio.1",
		.phy_addr = 0,
		.platform_data = &reh132_ar8216_data,
	},
};

static void __init reh132_gmac_setup(void)
{
	void __iomem *base;
	u32 t;

	base = ioremap(AR934X_GMAC_BASE, AR934X_GMAC_SIZE);

	t = __raw_readl(base + AR934X_GMAC_REG_ETH_CFG);
	t &= ~(AR934X_ETH_CFG_RGMII_GMAC0 | AR934X_ETH_CFG_MII_GMAC0 |
	       AR934X_ETH_CFG_GMII_GMAC0 | AR934X_ETH_CFG_SW_ONLY_MODE);
	t |= AR934X_ETH_CFG_RGMII_GMAC0 | AR934X_ETH_CFG_SW_ONLY_MODE;

	__raw_writel(t, base + AR934X_GMAC_REG_ETH_CFG);

	iounmap(base);
}

static void __init reh132_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	ath79_gpio_output_select(REH132_GPIO_LED_USB, AR934X_GPIO_OUT_GPIO);
	ath79_register_m25p80(NULL);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(reh132_leds_gpio),
				 reh132_leds_gpio);
	ath79_register_gpio_keys_polled(-1, REH132_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(reh132_gpio_keys),
					reh132_gpio_keys);
	ath79_register_usb();
	ath79_register_wmac(art + REH132_WMAC_CALDATA_OFFSET, NULL);
	ap91_pci_init(art + REH132_PCIE_CALDATA_OFFSET, NULL);

	reh132_gmac_setup();

	ath79_register_mdio(1, 0x0);

	ath79_init_mac(ath79_eth0_data.mac_addr, art + REH132_MAC0_OFFSET, 0);

	mdiobus_register_board_info(reh132_mdio0_info,
				    ARRAY_SIZE(reh132_mdio0_info));

	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_RGMII;
	ath79_eth0_data.phy_mask = BIT(0);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio1_device.dev;
	ath79_eth0_pll_data.pll_1000 = 0x06000000;
	ath79_register_eth(0);

	/* GMAC1 is connected to the internal switch */
	ath79_init_mac(ath79_eth1_data.mac_addr, art + REH132_MAC1_OFFSET, 0);
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
	ath79_eth1_data.speed = SPEED_100;
	ath79_eth1_data.duplex = DUPLEX_FULL;

	ath79_register_eth(1);
}

MIPS_MACHINE(ATH79_MACH_REH132, "REH132", "Atheros REH132 reference board",
	     reh132_setup);
