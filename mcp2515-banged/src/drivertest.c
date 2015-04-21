#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

static int __init drivertest_probe(struct platform_device *dev) {
	printk(KERN_ALERT "probe device: %s\n", dev->name);
	return 0;
}

static int drivertest_remove(struct platform_device *dev) {
	printk(KERN_ALERT "remove device: %s\n", dev->name);
	return 0;
}

static void drivertest_device_release(struct device *dev) {
	printk(KERN_ALERT "device release\n");
}

static struct platform_driver drivertest_driver = {
	.driver = {
		.name = "drivertest",
		.owner = THIS_MODULE,
	},
	.probe = drivertest_probe,
	.remove = drivertest_remove,
};

static struct platform_device drivertest_device = {
	.name = "drivertest",
	.id = 0,
	.dev = {
		.release = drivertest_device_release,
	},
};

static int __init drivertest_init(void) {
	printk(KERN_ALERT "Driver test init\n");
	platform_device_register(&drivertest_device);
	platform_driver_register(&drivertest_driver);
	return 0;
}

static void __exit drivertest_exit(void) {
	printk(KERN_ALERT "Driver test exit\n");
	platform_driver_unregister(&drivertest_driver);
	platform_device_unregister(&drivertest_device);
}

module_init(drivertest_init);
module_exit(drivertest_exit);
MODULE_LICENSE("GPL");

