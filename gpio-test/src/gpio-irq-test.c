#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#define DRV_NAME "GPIO Test"

static unsigned int irq_number;
static unsigned int gpio_number = 0;
module_param(gpio_number, uint, 0);
MODULE_PARM_DESC(gpio_number, "GPIO");

static irqreturn_t gpio_reset_interrupt(int irq, void* dev_id) {
        printk(KERN_INFO "IRQ %d event (GPIO %d)\n", irq_number, gpio_number);
        return(IRQ_HANDLED);
}

static int __init mymodule_init(void) {
    int err;

    if (gpio_request(gpio_number, "GPIO IRQ TEST")) {
        printk (KERN_ERR "can't get GPIO %d\n", gpio_number);
        return -1;
    } else {
        printk (KERN_INFO "requested GPIO %d\n", gpio_number);
    }

    if ((irq_number=gpio_to_irq(gpio_number))<0) {
        printk(KERN_ERR "can't map GPIO %d to IRQ\n",irq_number);
        err = irq_number;
	goto GPIO_IRQ_ERR;
    }
    
    if ( request_irq(irq_number, gpio_reset_interrupt, IRQF_TRIGGER_FALLING|IRQF_ONESHOT, "gpio_irq_reset", NULL) ) {
         printk(KERN_ERR "GPIO IRQ Test: trouble requesting IRQ %d\n",irq_number);
         err = -1;
         goto IRQ_REQUEST_ERR;
    } else {
         printk(KERN_INFO "GPIO IRQ Test: requested IRQ %d for GPIO %d-> fine\n", gpio_number, irq_number);
    }
    return 0;

IRQ_REQUEST_ERR:

GPIO_IRQ_ERR:
    gpio_free(gpio_number);

    return(err);

}

static void __exit mymodule_exit(void) {
   printk (KERN_INFO "freeing GPIO %d IRQ %d\n", gpio_number, irq_number);
   gpio_free(gpio_number);
   free_irq(irq_number, NULL );
   return;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mister X");
