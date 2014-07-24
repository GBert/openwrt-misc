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
#include <linux/version.h>
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
u16 spi_devices_bus[MAX_DEVICES];
u16 spi_devices_cs[MAX_DEVICES];
static int spi_devices_count=0;

static void register_device(char *devdesc);
static void release_device(u16 bus, u16 cs,struct spi_device * dev);

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
			release_device(spi_devices_bus[i],spi_devices_cs[i],spi_devices[i]);
			spi_devices[i]=NULL;
		}
	}
	/* and return */
	return;
}
module_exit(spi_config_exit);

int spi_config_match_cs(struct device * dev, void *data) {
	/* convert pointers to something we need */
	struct spi_device *sdev=(struct spi_device *)dev;
	u8 cs=(int)data;
	/* convert to SPI device */
	printk(KERN_INFO " spi_config_match_cs: SPI%i: check CS=%i to be %i\n",sdev->master->bus_num,sdev->chip_select,cs);

	if (sdev->chip_select == cs) {
		if (dev->driver) {
			printk(KERN_INFO " spi_config_match_cs: SPI%i.%i: Found a device with modinfo %s\n",sdev->master->bus_num,sdev->chip_select,dev->driver->name);
		} else {
			printk(KERN_INFO " spi_config_match_cs: SPI%i.%i: Found a device, but no driver...\n",sdev->master->bus_num,sdev->chip_select);
		}
		return 1;
	}
	/* by default return 0 - no match */
	return 0;
}

static void register_device(char *devdesc) {
	char *tmp;
	int i;
	/* the board we configure */
	struct spi_board_info *brd;
	int brd_irq_gpio; /* an info that is not in the board */
	u32 pd_len=0; /* the length of the platform data */
	int force_release=0; /* flag for removing previous module - if not allocated by us...*/

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
			/* some keyonly fields */
			if (strcmp(key,"force_release")==0) {
				force_release=1;
				continue;
			} else {
				printk(KERN_ERR " spi_config_register: incomplete argument: %s - no value\n",key);
				goto register_device_err;
			}
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
			if (kstrtou16(value,10,&brd->mode)) {
				printk(KERN_ERR " spi_config_register: %s=%s can not get parsed - ignoring config\n",key,value);
				goto register_device_err;
			}
		} else if (strcmp(key,"modalias")==0) {
			strncpy(brd->modalias,value,sizeof(brd->modalias));
		} else if (strcmp(key,"pd")==0) {
			/* we may only allocate once */
			if (pd_len) {
				printk(KERN_ERR " spi_config_register: the pd has already been configured - ignoring config\n");
				goto register_device_err;
			}
			/* get the length of platform data */
			if (kstrtou32(value,0,&pd_len)) {
				printk(KERN_ERR " spi_config_register: the pd length can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			/* now we allocate it - maybe we should allocate a minimum size to avoid abuse? */
			brd->platform_data=kmalloc(pd_len,GFP_KERNEL);
			if (brd->platform_data) {
				memset((char*)brd->platform_data,0,pd_len);
			} else {
				printk(KERN_ERR " spi_config_register: could not allocate %i bytes for platform memory\n",pd_len);
				goto register_device_err;
			}
		} else if (strncmp(key,"pdx-",4)==0) {
			u32 offset;
			char *src=value;
			/* store an integer in pd */
			if (kstrtou32(key+4,0,&offset)) {
				printk(KERN_ERR " spi_config_register: the pdx position can not get parsed in %s - ignoring config\n",key+4);
				goto register_device_err;
			}
			if (offset>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pdx offset %i is outside of allocated length %i- ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* and now we fill it in with the data */
			while (offset<pd_len) {
				char hex[3];
				char v;
				hex[0]=*(src++);
				if (!hex[0]) { break; }
				hex[1]=*(src++);
				if (!hex[1]) {
					printk(KERN_ERR " spi_config_register: the pdx hex-data is not of expected length in %s (hex number needs to be chars)- ignoring config\n",
						value);
					goto register_device_err;
				}
				hex[2]=0; /* zero terminate it */
				if (kstrtou8(hex,16,&v)) {
					printk(KERN_ERR " spi_config_register: the pdx data could not get parsed for %s in %s - ignoring config\n",
						hex,value);
				} else {
					*((char*)(brd->platform_data+offset))=v;
					offset++;
				}
			}
			/* check overflow */
			if (*src) {
				printk(KERN_ERR " spi_config_register: the pdx data exceeds allocated length - rest of data is: %s - ignoring config\n",src);
				goto register_device_err;
			}
		} else if (strncmp(key,"pds64-",6)==0) {
			u32 offset;
			s64 v;
			/* store an integer in pd */
			if (kstrtou32(key+6,0,&offset)) {
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
			u32 offset;
			u64 v;
			/* store an integer in pd */
			if (kstrtou32(key+6,0,&offset)) {
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
			u32 offset;
			s32 v;
			/* store an integer in pd */
			if (kstrtou32(key+6,0,&offset)) {
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
			u32 offset;
			u32 v;
			/* store an integer in pd */
			if (kstrtou32(key+6,0,&offset)) {
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
			u32 offset;
			u16 v;
			/* store an integer in pd */
			if (kstrtou32(key+6,0,&offset)) {
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
			u32 offset;
			s16 v;
			/* store an integer in pd */
			if (kstrtou32(key+6,0,&offset)) {
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
			u32 offset;
			u8 v;
			/* store an integer in pd */
			if (kstrtou32(key+5,0,&offset)) {
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
		} else if (strncmp(key,"pdp-",4)==0) {
			u32 offset;
			u32 v;
			/* store an integer in pd */
			if (kstrtou32(key+4,0,&offset)) {
				printk(KERN_ERR " spi_config_register: the pdp position can not get parsed in %s - ignoring config\n",key+5);
				goto register_device_err;
			}
			if(offset+sizeof(void*)>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pdp position %02x is larger than the length of the structure (%02x) - ignoring config\n",offset,pd_len);
				goto register_device_err;
			}
			/* now read the value */
			if (kstrtou32(value,0,&v)) {
				printk(KERN_ERR " spi_config_register: the pdp value can not get parsed in %s - ignoring config\n",value);
				goto register_device_err;
			}
			/* and do some sanity checks */
			if (v>=pd_len) {
				printk(KERN_ERR " spi_config_register: the pdp value points outside of platform data - ignoring config\n");
				goto register_device_err;
			}
			/* maybe we also should check that there is at least sizeof(void*) bytes left to point to...*/
			*((char**)(brd->platform_data+offset))=(char*)(brd->platform_data)+v;
		} else {
			printk(KERN_ERR " spi_config_register: unsupported argument %s - ignoring config\n",key);
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
	/* check if a device exists already for the requested cs - but is not allocated by us...*/
	if (master) {
		struct device *found=device_find_child(&master->dev,(void*)(int)brd->chip_select,spi_config_match_cs);
		if (found) {
			printk(KERN_ERR "spi_config_register:spi%i.%i:%s: found already registered device\n", brd->bus_num,brd->chip_select,brd->modalias);
			if (force_release) {
				/* write the message */
				printk(KERN_ERR " spi_config_register:spi%i.%i:%s: forcefully-releasing already registered device taints kernel\n", brd->bus_num,brd->chip_select,brd->modalias);
				/* let us taint the kernel */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,9,0)
				add_taint(TAINT_FORCED_MODULE);
#else
				add_taint(TAINT_FORCED_MODULE,LOCKDEP_STILL_OK);
#endif
				/* the below leaves some unallocated memory wasting kernel memory !!! */
				spi_unregister_device((struct spi_device*)found);
				put_device(found);
			} else {
				printk(KERN_ERR " spi_config_register:spi%i.%i:%s: if you are sure you may add force_release to the arguments\n", brd->bus_num,brd->chip_select,brd->modalias);
				/* release device - needed from device_find_child */
				put_device(found);
				goto register_device_err;
			}
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
		spi_devices_bus[i]=spi_devices[spi_devices_count]->master->bus_num;
		spi_devices_cs[i]=spi_devices[spi_devices_count]->chip_select;

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
			char prefix[64];
			snprintf(prefix,sizeof(prefix),"spi_config_register:spi%i.%i:platform data:",
				spi_devices[spi_devices_count]->master->bus_num,
				spi_devices[spi_devices_count]->chip_select
				);
			print_hex_dump(KERN_INFO,prefix,DUMP_PREFIX_ADDRESS,
				16,1,
				spi_devices[spi_devices_count]->dev.platform_data,pd_len,true
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

static void release_device(u16 bus, u16 cs,struct spi_device *spi) {
	/* checking if the device is still "ours" */
	struct spi_master *master=NULL;
	struct device *found=NULL;
	/* first the bus */
	master=spi_busnum_to_master(bus);
	if (!master) {
		printk(KERN_ERR " spi_config_register: no spi%i bus found - not deallocating\n",bus);
		return;
	}
	/* now check the device for the cs we keep on record */
	found=device_find_child(&master->dev,(void*)(int)cs,spi_config_match_cs);
	if (! found) {
		printk(KERN_ERR " spi_config_register: no spi%i.%i bus found - not deallocating\n",bus,cs);
		return;
	}
	/* and compare if it is still the same as our own record */
	if (found != &spi->dev) {
		printk(KERN_ERR " spi_config_register: the device spi%i.%i is different from the one we allocated - not deallocating\n",bus,cs);
		return;
	}

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

