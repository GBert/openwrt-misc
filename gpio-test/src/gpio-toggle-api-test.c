#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/mm.h>

#define NR_REPEAT	1*1000*1000UL

#define DRV_NAME "GPIO toggle speed test"

static unsigned int gpio = 0;
module_param(gpio, uint, 0);
MODULE_PARM_DESC(gpio, "GPIO");

static int __init mymodule_init(void) {
    unsigned long t1, t2, t_diff_msec;

    uint32_t repeat;

    if (gpio_request(gpio, "GPIO toggle speed test")) {
        printk (KERN_ERR "can't get GPIO %d\n", gpio);
        return -1;
    } else
        printk (KERN_INFO "requested GPIO %d\n", gpio);

    if (gpio_direction_output(gpio,0)) {
        printk (KERN_ERR "can't set GPIO %d as output\n", gpio);
        printk (KERN_INFO "freeing GPIO %d\n", gpio);
        gpio_free(gpio);
        return -1;
    }

    printk (KERN_INFO "start toggling GPIO %d\n", gpio);
    t1 = jiffies;
    for(repeat=0;repeat<NR_REPEAT;repeat++) {
	gpio_set_value(gpio,1);
	gpio_set_value(gpio,0);
    }
    t2 = jiffies;
    t_diff_msec = ((long)t2 - (long)t1 ) * 1000/HZ;
    printk (KERN_INFO "  end toggling GPIO %d - %ld toggles in %ld msec -> %ld kHz\n",
                          gpio, NR_REPEAT, t_diff_msec, NR_REPEAT/t_diff_msec );
    return(0);
}

static void __exit mymodule_exit(void) {
   printk (KERN_INFO "freeing GPIO %d\n", gpio);
   gpio_free(gpio);
   return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mister X");
