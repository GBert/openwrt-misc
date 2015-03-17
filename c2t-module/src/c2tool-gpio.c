/*
 *  Silicon Labs C2 port Linux support for c2tool 
 *
 *  Copyright (c) 2015 Gerhard Bertelsmann <info@gerhard-bertelsmann.de>
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

static int c2ck = 19;
static int c2d = 20;

module_param(c2ck, int, S_IRUSR);
MODULE_PARM_DESC(c2ck, "C2CK pin");
module_param(c2d, int, S_IRUSR);
MODULE_PARM_DESC(c2d, "C2D pin");

static void gpio_c2port_access(int status)
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

static void gpio_c2port_c2d_dir(int dir)
{
	if (dir)
		gpio_direction_input(c2d);
	else
		gpio_direction_output(c2d, gpio_get_value(c2d));
}

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

	return 0;

free_gpio:
	gpio_free(c2ck);
exit:
	return ret;
}

static void __exit gpio_c2port_exit(void)
{
	/* Setup the GPIOs as input by default (access = 0) */
	gpio_free(c2ck);
	gpio_free(c2d);
}

module_init(gpio_c2port_init);
module_exit(gpio_c2port_exit);

MODULE_AUTHOR("Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_DESCRIPTION("Silicon Labs C2 port Linux support for c2tool");
MODULE_LICENSE("GPL");
