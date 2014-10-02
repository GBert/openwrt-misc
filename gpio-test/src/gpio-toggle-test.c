#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#define NR_REPEAT	1*1000*1000UL

#define DRV_NAME "GPIO toggle speed test"

static unsigned int gpio = 0;
module_param(gpio, uint, 0);
MODULE_PARM_DESC(gpio, "GPIO");

static int __init mymodule_init(void) {
    unsigned long t1, t2, t_diff_msec;
    void __iomem *set_reg   = ath79_gpio_base + AR71XX_GPIO_REG_SET;
    void __iomem *clear_reg = ath79_gpio_base + AR71XX_GPIO_REG_CLEAR;

    uint32_t mask, repeat;

/*    if (soc_is_ar724x()) {
        printk (KERN_INFO "right architecture ->fine\n");
    } else {
        printk (KERN_ERR "wrong architecture - must be AR933x\n");
        return -1;
    } */

    /* requesting port to be sure the port is free */
    if (gpio_request(gpio, "GPIO toggle speed test")) {
        printk (KERN_ERR "can't get GPIO %d\n", gpio);
        return -1;
    } else {
        printk (KERN_INFO "requested GPIO %d\n", gpio);
    }

    mask = 1 << gpio;

    printk (KERN_INFO "start toggling GPIO %d\n", gpio);
    t1 = jiffies;
    for(repeat=0;repeat<NR_REPEAT;repeat++)
    {
        __raw_writel(mask, set_reg);
        __raw_writel(mask, clear_reg);
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
