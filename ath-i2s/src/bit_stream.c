/*
 *  PoC  bit streaming device driver module using I2S
 *
 *  Copyright (C) 2013 Gerhard Bertelsmann
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
 
#include "bit_stream.h"
 
#define FIRST_MINOR 0
#define MINOR_CNT 1

#define DRV_NAME        "bit_stream"
#define DRV_DESC        "bit streaming using I2S"
#define DRV_VERSION     "0.01"

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
static int frequency;
 
static long bit_stream_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    struct bit_stream_arg_t q; 
    switch (cmd) {
        case BIT_STREAM_GET_FREQ:
            q.frequency = frequency;
            if (copy_to_user((struct bit_stream_arg_t *)arg, &q, sizeof(struct bit_stream_arg_t))) {
                return -EACCES;
            }
            break;
        case BIT_STREAM_SET_FREQ:
            if (copy_from_user(&q, (struct bit_stream_arg_t *)arg, sizeof(struct bit_stream_arg_t))) {

                return -EACCES;
            }
            frequency = q.frequency;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static ssize_t bit_stream_write(struct file *f, const char __user *buf, size_t len, loff_t *off) {
  printk(KERN_INFO "Driver: write()\n");
  return len;
}
 
static struct file_operations bit_stream_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = bit_stream_ioctl,
    .write = bit_stream_write,
};
 
static int __init bit_stream_init(void) {
    int ret;
    struct device *dev_ret;
 
    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "bit_stream")) < 0) {
        printk(KERN_ERR "Can't add bit_stream device\n");
        return ret;
    }
 
    cdev_init(&c_dev, &bit_stream_fops);
 
    if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0) {
        printk(KERN_ERR "Can't add char device\n");
        return ret;
    }
     
    if ((cl = class_create(THIS_MODULE, "bitstream")) == NULL ) {
        printk(KERN_ERR "Can't create chardev class\n");
        goto ERR_CLASS_CREATE;
    }
    if ((dev_ret = device_create(cl, NULL, dev, NULL, "bit_stream")) == NULL ) {
        printk(KERN_ERR "Can't create char device\n");
        goto ERR_DEV_CREATE;
    }
    printk(KERN_INFO "bit stream driver registered\n");
    return 0;

ERR_DEV_CREATE:
    class_destroy(cl);
ERR_CLASS_CREATE:
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
    return -1; 
}
 
static void __exit bit_stream_exit(void) {
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
    printk(KERN_INFO "bit stream driver unregistered\n");
}
 
module_init(bit_stream_init);
module_exit(bit_stream_exit);
 
MODULE_DESCRIPTION("bit stream driver");
MODULE_AUTHOR("Gerhard Bertelsmann info@gerhard-bertelsmann.de");
MODULE_LICENSE("GPL");
