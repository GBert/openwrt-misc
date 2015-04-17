/* CAN device driver skeleton */


#include <linux/can/core.h>
#include <linux/can/dev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>

#define TX_ECHO_SKB_MAX 1

struct xyz_priv {
	struct can_priv	   can;
	struct net_device *net;

	u8 *tx_buf;
	u8 *rx_buf;
	int irq;

	struct sk_buff *tx_skb;
	int tx_len;
};

static int xyz_open(struct net_device *net) {
        printk(KERN_INFO "%s: \n", __func__);
	return open_candev(net);
}

static int xyz_stop(struct net_device *net) {
	close_candev(net);
        printk(KERN_INFO "%s: \n", __func__);
	return 0;
}

static netdev_tx_t xyz_start_xmit(struct sk_buff *skb, struct net_device *net) {
        printk(KERN_INFO "%s: \n", __func__);
	return NETDEV_TX_OK;
}

static const struct net_device_ops xyz_netdev_ops = {
	.ndo_open = xyz_open,
	.ndo_stop = xyz_stop,
	.ndo_start_xmit = xyz_start_xmit,
	.ndo_change_mtu = can_change_mtu,
};

static int xyz_can_probe(struct platform_device *pdev) {
	struct net_device *net;
        printk(KERN_INFO "%s: \n", __func__);
	net = alloc_candev(sizeof(struct xyz_priv), TX_ECHO_SKB_MAX);
	if (!net)
		return -ENOMEM;
	return 0;
}

static int xyz_can_remove(struct platform_device *pdev) {
	struct net_device *net_dev = platform_get_drvdata(pdev);
        printk(KERN_INFO "%s: \n", __func__);

	unregister_candev(net_dev);
	free_candev(net_dev);

	return 0;
}

static int __maybe_unused xyz_can_suspend(struct device *device) {
	struct net_device *net_dev = dev_get_drvdata(device);
        printk(KERN_INFO "%s: \n", __func__);
	return 0;
}

static int __maybe_unused xyz_can_resume(struct device *device) {
	struct net_device *net_dev = dev_get_drvdata(device);
        printk(KERN_INFO "%s: \n", __func__);
	return 0;
}

static SIMPLE_DEV_PM_OPS(xyz_can_pm_ops, xyz_can_suspend, xyz_can_resume);

static struct platform_driver xyz_can_driver = {
	.driver = {
		.name = "xyz_can",
		.owner = THIS_MODULE,
	},
	.probe = xyz_can_probe,
	.remove = xyz_can_remove,
};

module_platform_driver(xyz_can_driver);

MODULE_AUTHOR("Mister X");
MODULE_DESCRIPTION("CAN driver skeleton");
MODULE_LICENSE("GPL v2");
