/*
 *  linux/arch/arm/mach-bcm2708/spi_config.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define DRV_NAME        "spi_config"
#define DEFAULT_DRIVER "spidev"
#define DEFAULT_SPEED  500000
#define DEFAULT_PARAM DEFAULT_DRIVER":500000"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/irq.h>

#include <linux/string.h>
#include <linux/spi/spi.h>

#include <linux/can/platform/mcp251x.h>

/* the module parameters */
static int bus=0;
module_param(bus, int, S_IRUGO);
MODULE_PARM_DESC(bus, "SPI bus id - default: 0");
static char* dev0=DEFAULT_PARAM;
module_param(dev0, charp, S_IRUGO);
MODULE_PARM_DESC(dev0, "SPI device 0 driver - default: "DEFAULT_PARAM);
static char* dev1=DEFAULT_PARAM;
module_param(dev1, charp, S_IRUGO);
MODULE_PARM_DESC(dev1, "SPI device 1 driver - default: "DEFAULT_PARAM);

/* the actual devices we will register - need this information to unregister them as well */
static struct spi_board_info spi_board_devices[2];
static struct spi_device *spi_devices[2];

/* define the master */
struct spi_master *master=NULL;

/* the actual register code */
static struct spi_device *parameterToDevice(int,int,char*, struct spi_board_info*);


static int __init spi_config_init(void)
{
	master=spi_busnum_to_master(bus);
	if (!master) {
		printk(KERN_ERR "spi_config: no master for spi%d found\n", bus);
        }

	/* and register devices */
	spi_devices[0]=parameterToDevice(bus,0,dev0,&spi_board_devices[0]);
	if (!spi_devices[0]) { printk(KERN_ERR "spi_config_init: could not register spi%i.%i as %s",bus,0,dev0); return -ENODEV; }

	spi_devices[1]=parameterToDevice(bus,1,dev1,&spi_board_devices[1]);
	if (!spi_devices[1]) { printk(KERN_ERR "spi_config_init: could not register spi%i.%i as %s",bus,1,dev1); return -ENODEV; }

	/* and return OK */
        return 0;
}
module_init(spi_config_init);

static void __exit spi_config_exit(void)
{
	/* unregister devices */
	spi_unregister_device(spi_devices[0]);
	spi_unregister_device(spi_devices[1]);

	/* and return */
	return;
}
module_exit(spi_config_exit);

static void parameterToDevice_mcp251x(int bus, int dev,char* param,struct spi_board_info* brd);
static struct spi_device * parameterToDevice(int bus, int dev,char* param,struct spi_board_info* brd) {
	char* idx, *head;
	long int tmp;
	/* check dev */
	if (!param) { param=DEFAULT_PARAM; }
	if (!param[0]) { param=DEFAULT_PARAM; }
	/* clean the device structure with some defaults */
	memset(brd,0,sizeof(struct spi_board_info));
	brd->max_speed_hz=DEFAULT_SPEED;
	brd->bus_num=bus;
	brd->chip_select=dev;
	brd->mode=SPI_MODE_0;
	/* now check/set the parameters */
	head=param;
	/* the driver name */
	/* replace separator */
	idx=strchr(head,':');if (idx) { *idx=0; } 
	/* copy the driver name to the structure */
	strncpy(brd->modalias,head,sizeof(brd->modalias));
	/* restore terminator and return if subsequent zero or return if no terminator was found */
	if (idx) { *idx=':'; head=idx+1; } else { head=NULL;goto parameterToDevice_custom; }

	/* now parse the spi speed*/
	/* replace separator */
	idx=strchr(head,':');if (idx) { *idx=0; } 
	/* parse the head if not 0 */
	if (*head) {
		if (kstrtol(head,10,&tmp)) {
			printk(KERN_INFO " spi_config:spi%i.%i:%s: bad max_spi_speed in %s\n", bus,dev,brd->modalias,param);
			return NULL;
		} else {
			brd->max_speed_hz=tmp;
		}
	}
	printk(KERN_INFO " spi_config:spi%i.%i:%s: max spi speed=%d\n", bus,dev,brd->modalias,brd->max_speed_hz);
	/* restore terminator and return if subsequent zero or return if no terminator was found */
	if (idx) { *idx=':'; head=idx+1; } else { head=NULL;goto parameterToDevice_custom; }

	/* now parse the irq section*/
	/* replace separator */
	idx=strchr(head,':');if (idx) { *idx=0; } 
	/* parse the head if not 0 */
	if (*head) {
		if (kstrtol(head,10,&tmp)) {
			printk(KERN_INFO " spi_config:spi%i.%i:%s: bad GPIO for irq in %s\n", bus,dev,brd->modalias,param);
			return NULL;
		} else {
			brd->irq=gpio_to_irq(tmp);
			printk(KERN_INFO " spi_config:spi%i.%i:%s: got GPIO %ld mapped to irq %d\n", bus,dev,brd->modalias,tmp,brd->irq);
		}
	}
	/* restore terminator and return if subsequent zero or return if no terminator was found */
	if (idx) { *idx=':'; head=idx+1; } else { head=NULL;goto parameterToDevice_custom; };

	/* now we can check other stuff - depending on driver */
parameterToDevice_custom:
	if ((strcmp(brd->modalias,"mcp2515")==0)||(strcmp(brd->modalias,"mcp2515")==0)) {
		parameterToDevice_mcp251x(bus,dev,head,brd);
	}
	/* and register it */
	return spi_new_device(master,brd);
};

static void parameterToDevice_mcp251x(int bus, int dev,char* param,struct spi_board_info* brd) {
	long int clock=16000000;
	
	/* allocate platform data */
	struct mcp251x_platform_data *pd=kmalloc(sizeof(struct mcp251x_platform_data),GFP_KERNEL);
	if (!pd) { return; }
	brd->platform_data=pd;
	/* clear and fill it */
	memset(pd,0,sizeof(struct mcp251x_platform_data));
	pd->irq_flags              = IRQF_TRIGGER_FALLING|IRQF_ONESHOT;
	/* here we can now add additional arguments from brd */
	if ((param)&&(*param)) {
		if (kstrtol(param,10,&clock)) {
			printk(KERN_INFO " spi_config:spi%i.%i:%s: bad oscilator frequency: %s\n", bus,dev,brd->modalias,param);
		}
	}
	pd->oscillator_frequency   = clock;
	printk(KERN_INFO " spi_config:spi%i.%i:%s: got oscilator frequency of %ld hz\n", bus,dev,brd->modalias,clock);
}

/* the module description */
MODULE_DESCRIPTION("SPI board setup");
MODULE_AUTHOR("Martin Sperl <kernel@martin.sperl.org>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);

