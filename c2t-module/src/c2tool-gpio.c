/*
 *  Silicon Labs C2 port Linux support for c2tool 
 *
 *  Copyright (c) 2015 Gerhard Bertelsmann <info@gerhard-bertelsmann.de>
 *     module is inspired by gpio-bb from Darron Broad
 *                        and Dirk Eibachs c2tool
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
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#include "c2tool-gpio.h"

static struct cdev cdev;
static int c2ck = 19;
static int c2d = 20;
static int delay = 1;

module_param(c2ck, int, S_IRUSR);
MODULE_PARM_DESC(c2ck, "C2CK pin");
module_param(c2d, int, S_IRUSR);
MODULE_PARM_DESC(c2d, "C2D pin");
module_param(delay, int, S_IRUSR);
MODULE_PARM_DESC(c2d, "delay [us]");

static void c2port_c2d_dir(int dir)
{
	if (dir)
		gpio_direction_input(c2d);
	else
		gpio_direction_output(c2d, gpio_get_value(c2d));
}

static void c2port_reset(void)
{
	/* To reset the device we have to keep clock line low for at least
	 * 20us.
	 */
	local_irq_disable();
	gpio_set_value(c2ck, 0);
	udelay(25);
	gpio_set_value(c2ck, 1);
	local_irq_enable();

	udelay(1);
}

/* TODO : c2port_reset and c2port_strobe_ck are similar - only delay differs */
static void c2port_strobe_ck(void)
{
	/* hi-low-hi transition must be below 5us */
	local_irq_disable();
	gpio_set_value(c2ck, 0);
	udelay(delay);		/* TODO : probably we don't need any delay because pulse need to be 20ns */
	gpio_set_value(c2ck, 1);
	local_irq_enable();

	udelay(delay);
}

static void c2port_write_ar(u8 addr)
{
	int i;
	/* int addr_s = addr; */

	/* START field */
	c2port_strobe_ck();

	/* INS field (11b, LSB first) */
	c2port_c2d_dir(0);
	gpio_set_value(c2d, 1);
	c2port_strobe_ck();
	gpio_set_value(c2d, 1);
	c2port_strobe_ck();

	/* ADDRESS field */
	for (i = 0; i < 8; i++) {
		gpio_set_value(c2d, addr & 0x01);
		c2port_strobe_ck();

		addr >>= 1;
	}

	/* STOP field */
	c2port_c2d_dir(1);
	c2port_strobe_ck();
	/* printk(KERN_INFO "%s : addr 0x%02X\n", __func__, addr_s); */
}

static int c2port_read_ar(u8 * addr)
{
	int i;

	/* START field */
	c2port_strobe_ck();

	/* INS field (10b, LSB first) */
	c2port_c2d_dir(0);
	gpio_set_value(c2d, 0);
	c2port_strobe_ck();
	gpio_set_value(c2d, 1);
	c2port_strobe_ck();

	/* ADDRESS field */
	c2port_c2d_dir(1);
	*addr = 0;
	for (i = 0; i < 8; i++) {
		*addr >>= 1;	/* shift in 8-bit ADDRESS field LSB first */

		c2port_strobe_ck();
		if (gpio_get_value(c2d))
			*addr |= 0x80;
	}

	/* STOP field */
	c2port_strobe_ck();
	/* printk(KERN_INFO "%s : addr 0x%02X\n", __func__, *addr); */

	return 0;
}

static int c2port_write_dr(u8 data)
{
	int timeout, i;
	/* int data_s = data; */

	/* START field */
	c2port_strobe_ck();

	/* INS field (01b, LSB first) */
	c2port_c2d_dir(0);
	gpio_set_value(c2d, 1);
	c2port_strobe_ck();
	gpio_set_value(c2d, 0);
	c2port_strobe_ck();

	/* LENGTH field (00b, LSB first -> 1 byte) */
	gpio_set_value(c2d, 0);
	c2port_strobe_ck();
	gpio_set_value(c2d, 0);
	c2port_strobe_ck();

	/* DATA field */
	for (i = 0; i < 8; i++) {
		gpio_set_value(c2d, data & 0x01);
		c2port_strobe_ck();

		data >>= 1;
	}

	/* WAIT field */
	c2port_c2d_dir(1);
	timeout = 20;
	do {
		c2port_strobe_ck();
		if (gpio_get_value(c2d))
			break;

		udelay(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	/* STOP field */
	c2port_strobe_ck();
	/* printk(KERN_INFO "%s : data 0x%02X timeout %d", __func__, data_s, timeout); */

	return 0;
}

static int c2port_read_dr(u8 * data)
{
	int timeout, i;

	/* START field */
	c2port_strobe_ck();

	/* INS field (00b, LSB first) */
	c2port_c2d_dir(0);
	gpio_set_value(c2d, 0);
	c2port_strobe_ck();
	gpio_set_value(c2d, 0);
	c2port_strobe_ck();

	/* LENGTH field (00b, LSB first -> 1 byte) */
	gpio_set_value(c2d, 0);
	c2port_strobe_ck();
	gpio_set_value(c2d, 0);
	c2port_strobe_ck();

	/* WAIT field */
	c2port_c2d_dir(1);
	timeout = 20;
	do {
		c2port_strobe_ck();
		if (gpio_get_value(c2d))
			break;

		udelay(1);
	} while (--timeout > 0);
	if (timeout == 0)
		return -EIO;

	/* DATA field */
	*data = 0;
	for (i = 0; i < 8; i++) {
		*data >>= 1;	/* shift in 8-bit DATA field LSB first */

		c2port_strobe_ck();
		if (gpio_get_value(c2d))
			*data |= 0x80;
	}

	/* STOP field */
	c2port_strobe_ck();
	/* printk(KERN_INFO "%s : data 0x%02X\n", __func__, (unsigned int)*data); */

	return 0;
}

static long c2port_gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err;
	struct c2port_command c2data;

	if (_IOC_TYPE(cmd) != C2PORT_GPIO_MAJOR) {
		return -EINVAL;
	}
	if (_IOC_DIR(cmd) & _IOC_READ) {
		if (!access_ok(VERIFY_WRITE, arg, _IOC_SIZE(cmd))) {
			return -EFAULT;
		}
	}
	if (_IOC_DIR(cmd) & _IOC_WRITE) {
		if (!access_ok(VERIFY_READ, arg, _IOC_SIZE(cmd))) {
			return -EFAULT;
		}
	}
	switch (cmd) {
	case C2PORT_RESET:
		c2port_reset();
		/* printk(KERN_INFO "%s : C2PORT_RESET\n", __func__); */
		break;

	case C2PORT_WRITE_AR:
		if (copy_from_user(&c2data, (struct c2port_command *)arg, sizeof(c2data)) != 0)
			return -EFAULT;
		/* printk(KERN_INFO "%s : C2PORT_WRITE_AR  addr 0x%02X\n", __func__, (unsigned int)c2data.ar); */
		c2port_write_ar(c2data.ar);
		break;

	case C2PORT_READ_AR:
		if (copy_from_user(&c2data, (struct c2port_command *)arg, sizeof(c2data)) != 0)
			return -EFAULT;
		/* printk(KERN_INFO "%s : C2PORT_READ_AR ...\n", __func__); */
		err = c2port_read_ar(&c2data.ar);
		if (err)
			return err;
		if (copy_to_user((struct c2port_command *)arg, &c2data, sizeof(c2data)) != 0)
			return -EFAULT;
		/* printk(KERN_INFO "%s : C2PORT_READ_AR done  addr 0x%02X\n", __func__, (unsigned int)c2data.ar); */
		break;

	case C2PORT_WRITE_DR:
		if (copy_from_user(&c2data, (struct c2port_command *)arg, sizeof(c2data)) != 0)
			return -EFAULT;
		/* printk(KERN_INFO "%s : C2PORT_WRITE_DR ...\n", __func__); */
		err = c2port_write_dr(c2data.dr);
		if (err)
			return err;
		/* printk(KERN_INFO "%s : C2PORT_WRITE_DR done data 0x%02X\n", __func__, (unsigned int)c2data.dr); */
		break;

	case C2PORT_READ_DR:
		if (copy_from_user(&c2data, (struct c2port_command *)arg, sizeof(c2data)) != 0)
			return -EFAULT;
		err = c2port_read_dr(&c2data.dr);
		/* printk(KERN_INFO "%s : C2PORT_READ_DR ...\n", __func__); */
		if (err)
			return err;
		if (copy_to_user((struct c2port_command *)arg, &c2data, sizeof(c2data)) != 0)
			return -EFAULT;
		/* printk(KERN_INFO "%s : C2PORT_READ_DR done data 0x%02X\n", __func__, (unsigned int)c2data.dr); */
		break;
	}
	return 0;
}

struct file_operations c2port_gpio_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = c2port_gpio_ioctl,
};

static int __init c2port_init(void)
{
	int ret;

	ret = gpio_request(c2ck, "c2port clock");
	if (ret)
		goto exit;
	gpio_direction_output(c2ck,1);

	ret = gpio_request(c2d, "c2port data");
	if (ret)
		goto free_gpio;
	gpio_direction_input(c2d);
	printk(KERN_INFO "c2tool-gpio using GPIO%02d (c2ck) and GPIO%02d (c2d) delay %d us\n", c2ck, c2d, delay);

	cdev_init(&cdev, &c2port_gpio_fops);
	cdev.owner = THIS_MODULE;
	return cdev_add(&cdev, MKDEV(C2PORT_GPIO_MAJOR, 0), 1);

free_gpio:
	gpio_free(c2ck);
exit:
	return ret;
}

static void __exit c2port_exit(void)
{
	/* Setup the GPIOs as input by default (access = 0) */
	gpio_free(c2ck);
	gpio_free(c2d);

	cdev_del(&cdev);
}

module_init(c2port_init);
module_exit(c2port_exit);

MODULE_AUTHOR("Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_DESCRIPTION("Silicon Labs C2 port Linux support for c2tool");
MODULE_LICENSE("GPL");
