/*******************************************************************************
 *                                                                             *
 * Linux GPIO IRQ latency test                                                 *
 *                                                                             *
 * taken from https://github.com/gkaindl/linux-gpio-irq-latency-test           *
 *                                                                             *
 ******************************************************************************/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/ioport.h>
#include <asm/io.h>

#define DRV_NAME           "gpio-hw-irq-latency-test"

static int gpio_irq;
static unsigned int gpio_in = 21;
module_param(gpio_in, uint, 0);
MODULE_PARM_DESC(gpio_in, "GPIO_IN");

static unsigned int gpio_out = 22;
module_param(gpio_out, uint, 0);
MODULE_PARM_DESC(gpio_out, "GPIO_OUT");

static irqreturn_t gpio_irq_handler(int irq, void *dev_id) {

    gpio_set_value(gpio_out, 1);
    udelay(1);
    gpio_set_value(gpio_out, 0);

    return IRQ_HANDLED;
}

int __init mymodule_init(void) {
    int err;

    err = gpio_request_one(gpio_out, GPIOF_OUT_INIT_LOW, "gpio lt out");
    if (err < 0) {
	printk(KERN_ALERT DRV_NAME " : failed to request GPIO pin %d.\n", gpio_out);
	goto err_return;
    }

    err = gpio_request_one(gpio_in, GPIOF_IN, "gpio lt irq in");
    if (err < 0) {
	printk(KERN_ALERT DRV_NAME " : failed to request IRQ pin %d.\n", gpio_in);
	goto err_free_gpio_return;
    }

    err = gpio_to_irq(gpio_in);
    if (err < 0) {
	printk(KERN_ALERT DRV_NAME " : failed to get IRQ for pin %d.\n", gpio_in);
	goto err_free_irq_return;
    } else {
	gpio_irq = err;
	err = 0;
    }

    err =
	request_irq(gpio_irq, gpio_irq_handler, IRQ_TYPE_EDGE_RISING | IRQF_NO_SUSPEND | IRQF_FORCE_RESUME,
		    "gpio_irq_reset", NULL);
    if (err) {
	printk(KERN_ERR "GPIO IRQ latency test: trouble requesting IRQ %d error %d\n", gpio_irq, err);
	goto err_free_irq_return;
    } else {
	printk(KERN_INFO "GPIO IRQ latency test: requested IRQ %d for GPIO %d-> fine\n", gpio_irq, gpio_in);
    }

    return 0;

err_free_irq_return:
    gpio_free(gpio_in);
err_free_gpio_return:
    gpio_free(gpio_out);
err_return:
    return err;
}

void __exit mymodule_exit(void) {

    free_irq(gpio_irq, NULL);

    gpio_free(gpio_in);
    gpio_free(gpio_out);

    printk(KERN_INFO DRV_NAME " : unloaded IRQ latency test.\n");
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Mister X");
MODULE_DESCRIPTION("kernel module to test gpio irq latency with external hardware");
MODULE_VERSION("1.0");
