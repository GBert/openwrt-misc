#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define DRV_NAME "GPIO Test"

static int irq_number;
static unsigned int gpio = 0;
module_param(gpio, uint, 0);
MODULE_PARM_DESC(gpio, "GPIO");

static irqreturn_t gpio_reset_interrupt(int irq, void *dev_id) {
    printk(KERN_INFO "IRQ %d event (GPIO %d)\n", irq_number, gpio);
    return (IRQ_HANDLED);
}

static int __init mymodule_init(void) {
    int err;

    if (gpio_request(gpio, "GPIO IRQ TEST")) {
	printk(KERN_ERR "can't get GPIO %d\n", gpio);
	return -1;
    } else {
	printk(KERN_INFO "requested GPIO %d\n", gpio);
    }

    irq_number = gpio_to_irq(gpio);
    if (irq_number < 0) {
	printk(KERN_ERR "can't map GPIO %d to IRQ : error %d\n", gpio, irq_number);
	err = irq_number;
	goto GPIO_IRQ_ERR;
    } else {
	printk(KERN_INFO "got IRQ %d for GPIO %d\n", irq_number, gpio);
    }

    err = request_irq(irq_number, gpio_reset_interrupt, IRQ_TYPE_EDGE_FALLING, "gpio_irq_reset", NULL);
    if (err) {
	printk(KERN_ERR "GPIO IRQ Test: trouble requesting IRQ %d error %d\n", irq_number, err);
	goto IRQ_REQUEST_ERR;
    } else {
	printk(KERN_INFO "GPIO IRQ Test: requested IRQ %d for GPIO %d-> fine\n", irq_number, gpio);
    }
    return 0;

IRQ_REQUEST_ERR:

GPIO_IRQ_ERR:
    printk(KERN_INFO "freeing GPIO %d\n", gpio);
    gpio_free(gpio);

    return (err);

}

static void __exit mymodule_exit(void) {
    printk(KERN_INFO "freeing GPIO %d IRQ %d\n", gpio, irq_number);
    gpio_free(gpio);
    free_irq(irq_number, NULL);
    return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mister X");
