#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/mm.h>

#define NR_REPEAT	1*1000*1000UL

#ifdef ATH79_SOC
#define AR71XX_RESET_BASE	0x18060000
#define AR71XX_RESET_REG_REV_ID	0x90
#define GPIO_START_ADDR		0x18040000
#define GPIO_SIZE		0x20
#define GPIO_OFFS_READ		0x04
#define GPIO_OFFS_SET		0x0C
#define GPIO_OFFS_CLEAR		0x10
#endif

#ifdef MT7688
#define GPIO_START_ADDR		0x10000600
#define GPIO_SIZE		0xB0
#define GPIO_OFFS_SET		0x30
#define GPIO_OFFS_CLEAR		0x40
#endif

#ifdef RT305X
#define GPIO_START_ADDR		0x10000600
#define GPIO_SIZE		0x40
#define GPIO_OFFS_SET		0x2C
#define GPIO_OFFS_CLEAR		0x30
#endif

#define DRV_NAME "GPIO toggle speed test"

static unsigned int gpio = 0;
module_param(gpio, uint, 0);
MODULE_PARM_DESC(gpio, "GPIO");

static int __init mymodule_init(void) {
    unsigned long t1, t2, t_diff_msec;

    uint32_t mask, repeat;

    void __iomem *gpio_addr = NULL;
    void __iomem *gpio_setdataout_addr = NULL;
    void __iomem *gpio_cleardataout_addr = NULL;

#ifdef ATH79_SOC
    void __iomem *gpio_readdata_addr = NULL;
    void __iomem *cpu_id = NULL;
    cpu_id = ioremap(AR71XX_RESET_BASE + AR71XX_RESET_REG_REV_ID, 0x4);
#endif

    gpio_addr = ioremap(GPIO_START_ADDR, GPIO_SIZE);
    /* requesting port to be sure the port is free */
    if (gpio_request(gpio, "GPIO toggle speed test")) {
	printk(KERN_ERR "can't get GPIO %d\n", gpio);
	return -1;
    } else {
	printk(KERN_INFO "requested GPIO %d\n", gpio);
    }

    if (gpio_direction_output(gpio, 0)) {
	printk(KERN_ERR "can't set GPIO %d as output\n", gpio);
	printk(KERN_INFO "freeing GPIO %d\n", gpio);
	gpio_free(gpio);
	return -1;
    }
#ifdef ATH79_SOC
    gpio_readdata_addr = gpio_addr + GPIO_OFFS_READ;
#endif
    gpio_setdataout_addr = gpio_addr + GPIO_OFFS_SET;
    gpio_cleardataout_addr = gpio_addr + GPIO_OFFS_CLEAR;
    mask = 1 << gpio;

    printk(KERN_INFO
	   "GPIO phys addr 0x%08X gpio_addr 0x%08X gpio_setdataout_addr 0x%08X gpio_cleardataout_addr 0x%08X\n",
	   (unsigned int)GPIO_START_ADDR, (unsigned int)gpio_addr, (unsigned int)gpio_setdataout_addr,
	   (unsigned int)gpio_cleardataout_addr);
    printk(KERN_INFO "start toggling GPIO %d\n", gpio);
    t1 = jiffies;
    for (repeat = 0; repeat < NR_REPEAT; repeat++) {
	/* __raw_writel(mask, gpio_setdataout_addr); */
	/* in = __raw_readl(gpio_cleardataout_addr); */
	/* __raw_writel(mask, gpio_cleardataout_addr); */
	/* in = __raw_readl(gpio_cleardataout_addr); */

	/*
	   iowrite32(mask, gpio_setdataout_addr);
	   iowrite32(mask, gpio_cleardataout_addr);
	 */

	*(volatile unsigned long *)gpio_setdataout_addr = mask;
	*(volatile unsigned long *)gpio_cleardataout_addr = mask;

    }
    t2 = jiffies;
    t_diff_msec = ((long)t2 - (long)t1) * 1000 / HZ;
    printk(KERN_INFO "  end toggling GPIO %d - %ld toggles in %ld msec -> %ld kHz\n",
	   gpio, NR_REPEAT, t_diff_msec, NR_REPEAT / t_diff_msec);
    return (0);
}

static void __exit mymodule_exit(void) {
    printk(KERN_INFO "freeing GPIO %d\n", gpio);
    gpio_free(gpio);
    return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mister X");
