/* CAN device driver skeleton */


#include <linux/can/core.h>
#include <linux/can/dev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>

#define TX_ECHO_SKB_MAX 1

struct xyz_can_priv {
	struct can_priv	   can;
	struct net_device *net;
	struct device *dev;

	u8 *tx_buf;
	u8 *rx_buf;
	int irq;

	struct sk_buff *tx_skb;
	int tx_len;
};

static int xyz_can_open(struct net_device *net) {
	struct xyz_can_priv *priv = netdev_priv(net);
        dev_info(priv->dev, "open event");
	return open_candev(net);
}

static int xyz_can_stop(struct net_device *net) {
	struct xyz_can_priv *priv = netdev_priv(net);
        dev_info(priv->dev, "close event");
	close_candev(net);
	return 0;
}

static netdev_tx_t xyz_start_can_xmit(struct sk_buff *skb, struct net_device *net) {
	struct xyz_can_priv *priv = netdev_priv(net);
        dev_info(priv->dev, "start xmit event");
	return NETDEV_TX_OK;
}

static const struct net_device_ops xyz_netdev_ops = {
	.ndo_open = xyz_can_open,
	.ndo_stop = xyz_can_stop,
	.ndo_start_xmit = xyz_start_can_xmit,
	/* .ndo_change_mtu = can_change_mtu, */
};

static int xyz_can_probe(struct platform_device *pdev) {
	struct net_device *net = NULL;
	struct xyz_can_priv *priv = NULL;
	struct device *dev = &pdev->dev;
	printk(KERN_INFO "%s called\n", __func__);
	dev_info(&pdev->dev, "probe event");

	net = alloc_candev(sizeof(struct xyz_can_priv), TX_ECHO_SKB_MAX);
	if (!net) {
		dev_err(&pdev->dev, "calloc_candev failed");
		return -ENOMEM;
	}
	return 0;
}

static int xyz_can_remove(struct platform_device *pdev) {
	struct net_device *net_dev = platform_get_drvdata(pdev);
	printk(KERN_INFO "%s called\n", __func__);
        dev_info(&pdev->dev, "remove event");

	unregister_candev(net_dev);
	free_candev(net_dev);

	return 0;
}

static struct platform_driver xyz_can_driver = {
	.driver = {
		.name = KBUILD_MODNAME,
		.owner = THIS_MODULE,
	},
	.probe = xyz_can_probe,
	.remove = xyz_can_remove,
	.suspend = NULL,
	.resume = NULL,
};

static int __init xyz_can_init(void) {
	int ret;
	ret = platform_driver_register(&xyz_can_driver);
	printk(KERN_INFO "%s called - return code %d\n", __func__, ret);
	return ret;
}
module_init(xyz_can_init);

static void __exit xyz_can_exit(void) {
	printk(KERN_INFO "%s called\n", __func__);
	platform_driver_unregister(&xyz_can_driver);
}
module_exit(xyz_can_exit);

MODULE_AUTHOR("Mister X");
MODULE_DESCRIPTION("CAN driver skeleton");
MODULE_LICENSE("GPL v2");
