/*
  gpio-proxy driver

  WARNING!!! THIS IS TOTALLY INSECURE - DO NOT USE ON PRODUCTION SYSTEMS
  UNLESS YOU FULLY UNDERSTAND THE IMPLICATIONS!

  This driver is part of the gpio-proxy package.  It is designed 
  to perform kernel gpio pin operations under the control of a 
  userland processes.  Operations currently supported are:
  
  - gpio_request()
  - gpio_free()
  - gpio_get_value()
  - gpio_set_value()
  - gpio_direction_input()
  - gpio_direction_output()
  
  You can use this to write basic device drivers in high level 
  languages like Perl/Python/C++, by writing the logic in a userland
  process, and relaying pin reads and writes to the driver for
  low-level activities.  Performance will be inferior to a *real*
  driver, but this is primarily intended for research and protoyping
  particularly where the target is an embedded system.
  
  The driver uses MAJOR:10 and MINOR:152 with a device called 
  /dev/gpio_proxy.  This device will be created automatically when 
  loading the driver on most systems.  Commands are sent to the 
  /dev/gpio_proxy device via ioctl.

  I plan to use this to develop an HD44780 driver.
   
  Copyright (C) bifferos@yahoo.co.uk, 2008
 */


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>


#define GPIO_MINOR 152

MODULE_AUTHOR("bifferos");
MODULE_LICENSE("GPL");


struct ControlPacket
{
  char operation;            // Letter indicating operation (R/F/G/S)
  char error;                // Not used - error returned in ioctl
  unsigned char rval;        // Used to return the value, if reading 
  unsigned char wval;        // Contains the value, if writing
  unsigned long gpio;        // The logical number of the gpio pin
  unsigned char text[16];    // Name of the pin, null terminated only used if requesting.
} ControlPacket;

static struct ControlPacket req;


// ioctl - I/O control
static int gpio_proxy_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg) 
{

  int i;
  unsigned long volatile* ptr;  
		
  // bail if it isn't the one-and-only ioctl (the command is encoded in the ioctl data).
  if (cmd != 1)
  {
    printk(KERN_INFO "Invalid ioctl on /dev/gpio_proxy\n");
    return -EINVAL;
  }

  // Read the request header
  if (copy_from_user(&req, (int *)arg, sizeof(req)))
  {
    printk(KERN_INFO "copy_from_user error on gpio_proxy request read.\n");
    return -EFAULT;
  }
  
  switch (req.operation)
  {
    case 'G': // reading data
      req.rval = gpio_get_value(req.gpio);      
      break;

    case 'S': // for writing data - no return value, so no error possible.
      gpio_set_value(req.gpio, req.wval);      
      break;

    case 'R': // for requesting a pin
      req.text[sizeof(req.text)-1] = 0;  // ensure text is null-terminated
      req.rval = 0;
      if (0 == gpio_request(req.gpio, req.text))
      {
        req.rval = 1;
      }
      break;
      
    case 'F': // To indicate pin is no longer required.
      gpio_free(req.gpio);
      break;

    case 'I': // To indicate pin to be used as input
      req.rval = 0;
      if (0 == gpio_direction_input(req.gpio))
      {
        req.rval = 1;
      }
      break;

    case 'O': // To indicate pin to be used as an output
      req.rval = 0;
      if (0 == gpio_direction_output(req.gpio, req.wval))
      {
        req.rval = 1;
      }
      break;

    default:
      printk(KERN_INFO "Unrecognised gpio operation.\n");
      return -EINVAL;
      
  }

  // Pass down the result.
  if (copy_to_user((int *)arg, &req, 4))
  {
    printk(KERN_INFO "copy_to_user error on pin request\n");
    return -EFAULT;
  }

  return 0;
}


static struct file_operations gpio_proxy_fops = {
	.owner		= THIS_MODULE,
	//.ioctl	= gpio_proxy_ioctl,
	.unlocked_ioctl	= gpio_proxy_ioctl,
};

static struct miscdevice gpio_proxy_device = {
	GPIO_MINOR,
	"gpio_proxy",
	&gpio_proxy_fops,
};

static int __init gpio_proxy_init(void)
{
	if (misc_register(&gpio_proxy_device)) {
		printk(KERN_WARNING "Couldn't register device %d\n", GPIO_MINOR);
		return -EBUSY;
	}
	printk(KERN_INFO "gpio-proxy research driver (v1.0) by bifferos, loaded.\n");
	return 0;
}

static void __exit gpio_proxy_exit(void)
{
	misc_deregister(&gpio_proxy_device);
	printk(KERN_INFO "gpio-proxy research driver (v1.0) by bifferos, unloaded.\n");
}

module_init(gpio_proxy_init);
module_exit(gpio_proxy_exit);


