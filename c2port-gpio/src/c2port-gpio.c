/*
 *  Silicon Labs C2 port Linux support for GPIO
 *
 *  Copyright (c) 2007 Rodolfo Giometti <giometti@linux.it>
 *  Copyright (c) 2007 Eurotech S.p.A. <info@eurotech.it>
 *  Copyright (c) 2014 Gerhard Bertelsmann <info@gerhard-bertelsmann.de>
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

static int c2ck = 1;
static int c2d = 0;

module_param(c2ck, int, S_IRUSR);
MODULE_PARM_DESC(c2ck, "C2CK pin");
module_param(c2d, int, S_IRUSR);
MODULE_PARM_DESC(c2d, "C2D pin");

/* --- C2 port operations --------------------------------------------------- */

static void gpio_c2port_access(struct c2port_device *dev, int status)
{
	if (status) {
		gpio_direction_output(c2ck, 1);
		gpio_direction_output(c2d, 1);
	} else {
		/* When access is "off" is important that both lines are set
		 * as inputs or hi-impedence */
		gpio_direction_input(c2ck);
		gpio_direction_input(c2d);
	}
}

static void gpio_c2port_c2d_dir(struct c2port_device *dev, int dir)
{
	if (dir)
		gpio_direction_input(c2d);
	else
		gpio_direction_output(c2d, gpio_get_value(c2d));
}

static int gpio_c2port_c2d_get(struct c2port_device *dev)
{
	return gpio_get_value(c2d);
}

static void gpio_c2port_c2d_set(struct c2port_device *dev, int status)
{
	gpio_set_value(c2d, status);
}

static void gpio_c2port_c2ck_set(struct c2port_device *dev, int status)
{
	gpio_set_value(c2ck, status);
}

static struct c2port_ops gpio_c2port_ops = {
	block_size	:512,		/* bytes */
	blocks_num	:30,		/* total flash size: 15360 bytes */
	access		:gpio_c2port_access,
	c2d_dir		:gpio_c2port_c2d_dir,
	c2d_get		:gpio_c2port_c2d_get,
	c2d_set		:gpio_c2port_c2d_set,
	c2ck_set	:gpio_c2port_c2ck_set,
};

static struct c2port_device *gpio_c2port_dev;

/* --- Module stuff --------------------------------------------------------- */

static int __init gpio_c2port_init(void)
{
	int ret;

	ret = gpio_request(c2ck, "c2port clock");
	if (ret)
		goto exit;
	gpio_direction_input(c2ck);

	ret = gpio_request(c2d, "c2port data");
	if (ret)
		goto free_gpio;
	gpio_direction_input(c2d);

	gpio_c2port_dev = c2port_device_register("uc", &gpio_c2port_ops, NULL);
	if (!gpio_c2port_dev)
		return -ENODEV;

	return 0;

free_gpio:
	gpio_free(c2ck);
exit:
	return ret;
}

static void __exit gpio_c2port_exit(void)
{
	/* Setup the GPIOs as input by default (access = 0) */
	gpio_c2port_access(gpio_c2port_dev, 0);

	c2port_device_unregister(gpio_c2port_dev);

	gpio_free(c2ck);
	gpio_free(c2d);
}

module_init(gpio_c2port_init);
module_exit(gpio_c2port_exit);

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_AUTHOR("Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_DESCRIPTION("Silicon Labs C2 port Linux support for GPIO");
MODULE_LICENSE("GPL");
