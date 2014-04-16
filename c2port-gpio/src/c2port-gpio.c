/*
 *  Silicon Labs C2 port Linux support for WR1100
 *
 *  Copyright (c) 2007 Rodolfo Giometti <giometti@linux.it>
 *  Copyright (c) 2007 Eurotech S.p.A. <info@eurotech.it>
 *
 *    s/wr1100_/gpio_/g
 *    s/WR1100_/GPIO_/g
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include <linux/c2port.h>

#define GPIO_C2CK 0
#define GPIO_C2D  1


/* --- C2 port operations --------------------------------------------------- */

static void gpio_c2port_access(struct c2port_device *dev, int status)
{
	if (status) {
		gpio_direction_output(GPIO_C2CK, 1);
		gpio_direction_output(GPIO_C2D, 1);
	}
	else {
		/* When access is "off" is important that both lines are set
		 * as inputs or hi-impedence */
		gpio_direction_input(GPIO_C2CK);
		gpio_direction_input(GPIO_C2D);
	}
}

static void gpio_c2port_c2d_dir(struct c2port_device *dev, int dir)
{
	if (dir)
		gpio_direction_input(GPIO_C2D);
	else
		gpio_direction_output(GPIO_C2D, gpio_get_value(GPIO_C2D));
}

static int gpio_c2port_c2d_get(struct c2port_device *dev)
{
	return gpio_get_value(GPIO_C2D);
}

static void gpio_c2port_c2d_set(struct c2port_device *dev, int status)
{
	gpio_set_value(GPIO_C2D, status);
}

static void gpio_c2port_c2ck_set(struct c2port_device *dev, int status)
{
	gpio_set_value(GPIO_C2CK, status);
}

static struct c2port_ops gpio_c2port_ops = {
	block_size	: 512, /* bytes */
	blocks_num	:  30, /* total flash size: 15360 bytes */

	access		: gpio_c2port_access,
	c2d_dir		: gpio_c2port_c2d_dir,
	c2d_get		: gpio_c2port_c2d_get,
	c2d_set		: gpio_c2port_c2d_set,
	c2ck_set	: gpio_c2port_c2ck_set,
};

static struct c2port_device *gpio_c2port_dev;

/* --- Module stuff --------------------------------------------------------- */

static int __init gpio_c2port_init(void)
{
	int ret;

	ret = gpio_request(GPIO_C2CK, "c2port clock");
	if (ret)
		goto exit;
	gpio_direction_input(GPIO_C2CK);

	ret = gpio_request(GPIO_C2D, "c2port data");
	if (ret)
		goto free_gpio;
	gpio_direction_input(GPIO_C2D);

	gpio_c2port_dev = c2port_device_register("uc",
					&gpio_c2port_ops, NULL);
	if (!gpio_c2port_dev)
		return -ENODEV;

	return 0;

free_gpio:
	gpio_free(GPIO_C2CK);
exit:
	return ret;
}

static void __exit gpio_c2port_exit(void)
{
	/* Setup the GPIOs as input by default (access = 0) */
	gpio_c2port_access(gpio_c2port_dev, 0);

	c2port_device_unregister(gpio_c2port_dev);

	gpio_free(GPIO_C2CK);
	gpio_free(GPIO_C2D);
}

module_init(gpio_c2port_init);
module_exit(gpio_c2port_exit);

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("Silicon Labs C2 port Linux support for GPIO");
MODULE_LICENSE("GPL");
