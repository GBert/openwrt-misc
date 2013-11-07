/*
 *  Bit serialization device driver module using I2S
 *
 *  Copyright (C) 2013 Gerhard Bertelsmann
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include "bit-serialization.h"

#define DRV_NAME	"bit-serializaion"
#define DRV_DESC	"Bit Serialization using I2S"
#define DRV_VERSION	"0.01"

#define BIT_SERIALIZATION_MAJOR_NUMBER (244)

static struct bit_serialization_platform_data *bit_serialization = NULL;

/*
 * I2S Output Pins
 *   including clock
 */

static int I2S_CLOCK = 0; 
module_param(I2S_CLOCK, int, 0644);
MODULE_PARM_DESC(I2S_CLOCK, "I2S_MCLK output pin (2 MHz)");

static int I2S_DATA = 0; 
module_param(I2S_DATA, int, 0644);
MODULE_PARM_DESC(I2S_DATA, "I2S_DATA output pin");


static int bit_serialization_open(struct inode *inode, struct file *icsp_file) {
    /* check if initialized */
    if (bit_serialization == NULL) return -ENODEV;

    /* call open callback */
    if (bit_serialization->open != NULL) bit_serialization->open(bit_serialization->data);

    /* done */
    return 0;
}

int bit_serialization_release(struct inode *inode, struct file *icsp_file) {
    /* check if initialized */
    if (bit_serialization == NULL) return -ENODEV;

    /* call close callback */
    if (bit_serialization->release != NULL) bit_serialization->release(bit_serialization->data);

    /* done */
    return 0;
}

int bit_serialization_device_ioctl(struct file *filep, unsigned int cmd, unsigned long data) {
    int err;
    unsigned int xfer_data = 0;
    /* TODO */
    err = 0;
    xfer_data = 1;
    return 0;
}

static const struct file_operations bit_serialization_fops = {
    .owner              = THIS_MODULE,
    .open               = bit_serialization_open,
    .release            = bit_serialization_release,
    .unlocked_ioctl     = bit_serialization_device_ioctl,
};

static int bit_serialization_probe(struct platform_device *pdev) {
    /* check platform data structure passed */
    if (pdev->dev.platform_data == NULL)
        return -ENXIO;

    /* save platform data (must be static) */
    bit_serialization = pdev->dev.platform_data;

    return 0;
}

static int bit_serialization_remove(struct platform_device *pdev) {
    return 0;
}

/* platform driver data */
static struct platform_driver bit_serialization_driver = {
    .driver    =
    {
        .name     = BIT_SERIALIZATION_DEVICE_NAME,
        .owner    = THIS_MODULE,
    },
    .probe        = bit_serialization_probe,
    .remove       = bit_serialization_remove,
};

static void bit_serialization_cleanup() {
    /* TODO */
}

static int __init bit_serialization_init(void) {
    return bit_serialization_probe();
}

static void __exit bit_serialization_exit(void) {
    bit_serialization_cleanup();
}

module_init(bit_serialization_init);
module_exit(bit_serialization_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Gerhard Bertelsmann <info@gerhard-bertelsmann.de");
MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
