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
#define DEFAULT_SPEED  500000
#define MAX_DEVICES 16

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

/* the module parameters */
static char* devices="";
module_param(devices, charp, S_IRUGO);
MODULE_PARM_DESC(devices, "SPI device configs");
/* the devices that we have registered */
static struct spi_device *spi_devices[MAX_DEVICES];
static int spi_devices_count=0;

static void register_device(char *devdesc);
static void release_device(struct spi_device * dev);

static int __init spi_config_init(void)
{
	char *head=devices;
	char *idx=NULL;
	/* clean up the spi_devices array */
	memset(spi_devices,0,sizeof(spi_devices));
	/* parse the devices parameter */
	while(*head) {
		/* find delimiter and create a separator */
		idx=strchr(head,',');if (idx) { *idx=0; }
		/* now parse the argument - if it is not "empty" */
		if (*head) { register_device(head); }
		/* and skip to next section and contine - exiting if there was no more ","*/
		if (idx) { head=idx+1; } else { break;}
	}
	/* and return OK */
        return 0;
}
module_init(spi_config_init);

static void __exit spi_config_exit(void)
{
	int i;
	/* unregister devices */
	for(i=0;i<MAX_DEVICES;i++) {
		if (spi_devices[i]) { 
			release_device(spi_devices[i]); 
			spi_devices[i]=NULL;
		}
	}
	/* and return */
	return;
}
module_exit(spi_config_exit);

static void register_device(char *devdesc) {
	char *tmp;
	int i;
	/* the board we configure */
	struct spi_board_info *brd;
	int brd_irq_gpio; /* an info that is not in the board */
	int pd_len=0; /* the length of the platform data */
	
	/* the master to which we connect */
	struct spi_master *master;
	/* log the parameter */
	printk(KERN_INFO "spi_config_register: device description: %s\n", devdesc);

	/* allocate the board and fill with defaults */
	brd=kmalloc(sizeof(struct spi_board_info),GFP_KERNEL);
	memset(brd,0,sizeof(struct spi_board_info));
        brd->bus_num=0xffff;
        brd->chip_select=-1;
	brd->max_speed_hz=DEFAULT_SPEED;
        brd->mode=SPI_MODE_0;
	brd->irq=-1;
	brd_irq_gpio=-1;

	/* now parse the device description */
	while((tmp=strsep(&devdesc,":"))) {
		char* key,*value;
		value=tmp;
		key=strsep(&value,"=");
		if (!value) {
			printk(KERN_ERR " spi_config_register: incomplete argument: %s - no value\n",key);
			goto register_device_err;
		}
		if (strcmp(key,"bus")==0) {
			if (kstrtos16(value,10,&brd->bus_num)) {
				printk(KERN_ERR " spi_config_register: %s=%s can not get parsed - ignoring config\n",key,value);
				goto register_device_err;
			}
		} else if (strcmp(key,"cs")==0) {
			if (kstrtou16(value,10,&brd->chip_select)) {
				printk(KERN_ERR " spi_config_register: %s=%s can not get parsed - ignoring config\n",key,value);
				goto register_device_err;
			}
		} else if (strcmp(key,"speed")==0) {
			if (kstrtoint(value,10,&brd->max_speed_hz)) {
				printk(KERN_ERR " spi_config_register: %s=%s can not get parsed - ignoring config\n",key,value);
				goto register_device_err;
			}
		} else if (strcmp(key,"gpioirq")==0) {
			if (kstrtoint(value,10,&brd_irq_gpio)) {
				printk(KERN_ERR " spi_config_register: %s=%s can not get parsed - ignoring config\n",key,value);
				goto register_device_err;
			}
			brd->irq=gpio_to_irq(brd_irq_gpio);
		} else if (strcmp(key,"irq")==0) {
			if (kstrtoint(value,10,&brd->irq)) {
				printk(KERN_ERR " spi_config_register: %s=%s can not get parsed - ignoring config\n",key,value);
				goto register_device_err;
			}
		} else if (strcmp(key,"mode")==0) {
			if (kstrtou8(value,10,&brd->mode)) {
				printk(KERN_ERR " spi_config_register: %s=%s can not get parsed - ignoring config\n",key,value);
				goto register_device_err;
			}
		} else if (strcmp(key,"modalias")==0) {
			strncpy(brd->modalias,value,sizeof(brd->modalias));
		} else if (strcmp(key,"pd")==0) {
			/* get the length of pd */
			u8 len=0;
			char hex[3];
			char *src=value;
			char *dst;
			/* copy hex from arg */
			hex[0]=*(src++);
			hex[1]=*(src++);
			hex[2]=0;
			if (kstrtou8(hex,16,&len)) {
				printk(KERN_ERR " spi_config_register: the pd length can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			pd_len=len;
			/* now we allocate it */
			brd->platform_data=dst=kmalloc(len,GFP_KERNEL);
			memset(dst,0,len);
			/* and now we fill it in with the rest of the data */
			while (len) {
				
				hex[0]=*(src++);
				if (!hex[0]) { break; }
				hex[1]=*(src++);
				if (!hex[1]) {
					printk(KERN_ERR " spi_config_register: the pd data is not of expected length in %s - ignoring config\n",
						value);
					goto register_device_err;
				} 
				if (kstrtou8(hex,16,dst)) {
					printk(KERN_ERR " spi_config_register: the pd data could not get parsed for %s in %s - ignoring config\n",
						hex,value);
				} else {
					dst++;len--;
				}
			}
		} else if (strncmp(key,"pds64-",6)==0) {
			u8 offset;
			s64 v;
			/* store an integer in pd */
			if (kstrtou8(key+6,16,&offset)) {
				printk(KERN_ERR " spi_config_register: the pds64 position can not get parsed in %s - ignoring config\n",key+6);
				goto register_device_err;
			}
			if(offset+7>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pds64 position %02x is larger than the length of the structure (%02x) - ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* now read the value */
			if (kstrtos64(value,0,&v)) {
				printk(KERN_ERR " spi_config_register: the pds64 value can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			*((s64*)(brd->platform_data+offset))=v;
		} else if (strncmp(key,"pdu64-",6)==0) {
			u8 offset;
			u64 v;
			/* store an integer in pd */
			if (kstrtou8(key+6,16,&offset)) {
				printk(KERN_ERR " spi_config_register: the pdu64 position can not get parsed in %s - ignoring config\n",key+6);
				goto register_device_err;
			}
			if(offset+7>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pdu64 position %02x is larger than the length of the structure (%02x) - ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* now read the value */
			if (kstrtou64(value,0,&v)) {
				printk(KERN_ERR " spi_config_register: the pds64 value can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			*((u64*)(brd->platform_data+offset))=v;
		} else if (strncmp(key,"pds32-",6)==0) {
			u8 offset;
			s32 v;
			/* store an integer in pd */
			if (kstrtou8(key+6,16,&offset)) {
				printk(KERN_ERR " spi_config_register: the pds32 position can not get parsed in %s - ignoring config\n",key+6);
				goto register_device_err;
			}
			if(offset+3>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pds32 position %02x is larger than the length of the structure (%02x) - ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* now read the value */
			if (kstrtos32(value,0,&v)) {
				printk(KERN_ERR " spi_config_register: the pds32 value can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			*((s32*)(brd->platform_data+offset))=v;
		} else if (strncmp(key,"pdu32-",6)==0) {
			u8 offset;
			u32 v;
			/* store an integer in pd */
			if (kstrtou8(key+6,16,&offset)) {
				printk(KERN_ERR " spi_config_register: the pdu32 position can not get parsed in %s - ignoring config\n",key+6);
				goto register_device_err;
			}
			if(offset+3>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pdu32 position %02x is larger than the length of the structure (%02x) - ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* now read the value */
			if (kstrtou32(value,0,&v)) {
				printk(KERN_ERR " spi_config_register: the pds32 value can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			*((u32*)(brd->platform_data+offset))=v;
		} else if (strncmp(key,"pdu16-",6)==0) {
			u8 offset;
			u16 v;
			/* store an integer in pd */
			if (kstrtou8(key+6,16,&offset)) {
				printk(KERN_ERR " spi_config_register: the pdu16 position can not get parsed in %s - ignoring config\n",key+6);
				goto register_device_err;
			}
			if(offset+1>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pdu16 position %02x is larger than the length of the structure (%02x) - ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* now read the value */
			if (kstrtou16(value,0,&v)) {
				printk(KERN_ERR " spi_config_register: the pdu16 value can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			*((u16*)(brd->platform_data+offset))=v;
		} else if (strncmp(key,"pds16-",6)==0) {
			u8 offset;
			s16 v;
			/* store an integer in pd */
			if (kstrtou8(key+6,16,&offset)) {
				printk(KERN_ERR " spi_config_register: the pdu16 position can not get parsed in %s - ignoring config\n",key+6);
				goto register_device_err;
			}
			if(offset+1>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pdu16 position %02x is larger than the length of the structure (%02x) - ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* now read the value */
			if (kstrtos16(value,0,&v)) {
				printk(KERN_ERR " spi_config_register: the pdu16 value can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			*((s16*)(brd->platform_data+offset))=v;
		} else if (strncmp(key,"pdu8-",5)==0) {
			u8 offset;
			u8 v;
			/* store an integer in pd */
			if (kstrtou8(key+5,16,&offset)) {
				printk(KERN_ERR " spi_config_register: the pdu8 position can not get parsed in %s - ignoring config\n",key+5);
				goto register_device_err;
			}
			if(offset>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pdu8 position %02x is larger than the length of the structure (%02x) - ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* now read the value */
			if (kstrtou8(value,0,&v)) {
				printk(KERN_ERR " spi_config_register: the pdu8 value can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			*((u8*)(brd->platform_data+offset))=v;
		} else {
			printk(KERN_ERR " spi_config_register: unsupported argument %s\n",key);
			goto register_device_err;
		}
	}
	/* now check if things are set correctly */
	/* first the bus */
	if (brd->bus_num==0xffff) {
		printk(KERN_ERR " spi_config_register: bus not set - ignoring config \n");
		goto register_device_err;
	}
	/* getting the master info */
	master=spi_busnum_to_master(brd->bus_num);
	if (!master) {
		printk(KERN_ERR " spi_config_register: no spi%i bus found - ignoring config\n",brd->bus_num);
		goto register_device_err;
	}
	/* now the chip_select */
	if (brd->chip_select<0) {
		printk(KERN_ERR " spi_config_register:spi%i: cs not set - ignoring config\n",brd->bus_num);
		goto register_device_err;
	}
	if (brd->chip_select>master->num_chipselect) {
		printk(KERN_ERR " spi_config_register:spi%i: cs=%i not possible for this bus - max_cs= %i - ignoring config\n",
			brd->bus_num,brd->chip_select,master->num_chipselect);
		goto register_device_err;
	}
	/* check if we are not in the list of registered devices already */
	for(i=0;i<spi_devices_count;i++) {
		if (
			(spi_devices[i]) 
			&& (brd->bus_num==spi_devices[i]->master->bus_num)
			&& (brd->chip_select==spi_devices[i]->chip_select)
			) {
			printk(KERN_ERR " spi_config_register:spi%i.%i: allready assigned - ignoring config\n",
				brd->bus_num,brd->chip_select);
			return;
		}
	}
	/* now check modalias */
	if (!brd->modalias[0]) {
		printk(KERN_ERR " spi_config_register:spi%i.%i: modalias not set - ignoring config\n",
			brd->bus_num,brd->chip_select);
		goto register_device_err;
	}
	/* check speed is "reasonable" */
	if (brd->max_speed_hz<2048) {
		printk(KERN_ERR " spi_config_register:spi%i.%i:%s: speed is set too low at %i\n",
			brd->bus_num,brd->chip_select,brd->modalias,brd->max_speed_hz);
		goto register_device_err;
	}
	
	/* register the device */
	if ((spi_devices[spi_devices_count]=spi_new_device(master,brd))) {
		/* now report the settings */
		if (spi_devices[spi_devices_count]->irq<0) {
			printk(KERN_INFO "spi_config_register:spi%i.%i: registering modalias=%s with max_speed_hz=%i mode=%i and no interrupt\n",
				spi_devices[spi_devices_count]->master->bus_num,
				spi_devices[spi_devices_count]->chip_select,
				spi_devices[spi_devices_count]->modalias,
				spi_devices[spi_devices_count]->max_speed_hz,
				spi_devices[spi_devices_count]->mode
				);
		} else {
			printk(KERN_INFO "spi_config_register:spi%i.%i: registering modalias=%s with max_speed_hz=%i mode=%i and gpio/irq=%i/%i\n",
				spi_devices[spi_devices_count]->master->bus_num,
				spi_devices[spi_devices_count]->chip_select,
				spi_devices[spi_devices_count]->modalias,
				spi_devices[spi_devices_count]->max_speed_hz,
				spi_devices[spi_devices_count]->mode,
				brd_irq_gpio,
				spi_devices[spi_devices_count]->irq
				);
		}
		if (spi_devices[spi_devices_count]->dev.platform_data) {
			printk(KERN_INFO "spi_config_register:spi%i.%i:%s: platform data=%32ph\n",
				spi_devices[spi_devices_count]->master->bus_num,
				spi_devices[spi_devices_count]->chip_select,
				spi_devices[spi_devices_count]->modalias,
				spi_devices[spi_devices_count]->dev.platform_data
				);
		}
		spi_devices_count++;
	} else {
		printk(KERN_ERR "spi_config_register:spi%i.%i:%s: failed to register device\n", brd->bus_num,brd->chip_select,brd->modalias);
		goto register_device_err;
	}

	/* and return successfull */
	return;
	/* error handling code */
register_device_err:
	if (brd->platform_data) { kfree(brd->platform_data); }
	kfree(brd);
	return;
}

static void release_device(struct spi_device *spi) {
	printk(KERN_INFO "spi_config_unregister:spi%i.%i: unregister device with modalias %s\n", spi->master->bus_num,spi->chip_select,spi->modalias);
	/* unregister device */
	spi_unregister_device(spi);
	/* seems as if unregistering also means that the structures get freed as well - kernel crashes, so we do not do it */
}

/* the module description */
MODULE_DESCRIPTION("SPI board setup");
MODULE_AUTHOR("Martin Sperl <kernel@martin.sperl.org>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);

