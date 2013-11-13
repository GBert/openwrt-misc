/*
 *  Atheros AR933X SoC built-in UART driver
 *
 *  Copyright (C) 2011 Gabor Juhos <juhosg@openwrt.org>
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/irq.h>

#define DRIVER_NAME "ar933x-uart"

#define AR71XX_APB_BASE		0x18000000
#define AR71XX_UART_SIZE        0x100
#define AR934X_UART1_BASE      (AR71XX_APB_BASE + 0x00500000)
#define AR934X_UART1_FIFO_SIZE	4

static struct resource ar933x_uart_resources[] = {
	{
		.start  = AR934X_UART1_BASE,
		.end    = AR934X_UART1_BASE + 0x14 - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = ATH79_MISC_IRQ(6),
		.end    = ATH79_MISC_IRQ(6),
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device ar933x_uart_device = {
	.name           = DRIVER_NAME,
	.id             = -1,
	.resource       = ar933x_uart_resources,
	.num_resources  = ARRAY_SIZE(ar933x_uart_resources),
};

static int __init ar933x_uart_reg_init(void)
{
	platform_device_register(&ar933x_uart_device);
	return 0;
}

static void __exit ar933x_uart_reg_exit(void)
{
	platform_device_unregister(&ar933x_uart_device);
}

module_init(ar933x_uart_reg_init);
module_exit(ar933x_uart_reg_exit);

MODULE_DESCRIPTION("Atheros HS AR934X UART driver register");
MODULE_AUTHOR("Mister X");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRIVER_NAME);
