// http://www.tldp.org/LDP/LG/issue93/bhaskaran.html


#define MODULE             
#define __KERNEL__	 
	
#include <linux/module.h>
#include <linux/config.h>
#include <linux/netdevice.h>
#include <linux/pci.h>

int econet_open (struct net_device *dev)
{
	printk("econet_open called\n");
	netif_start_queue (dev);
	return 0;
}

int econet_release (struct net_device *dev)
{
	printk ("econet_release called\n");
	netif_stop_queue(dev);
	return 0;
}

static int econet_xmit (struct sk_buff *skb, struct net_device *dev)
{
	printk ("dummy xmit function called....\n");
	dev_kfree_skb(skb);
	return 0;
}

int econet_init (struct net_device *dev)
{
	dev->open = econet_open;
	dev->stop = econet_release;
	dev->hard_start_xmit = econet_xmit;
	printk ("econet device initialized\n");
	return 0;
}

struct net_device econet = {init: econet_init};

int econet_init_module (void)
{
	int result;

	strcpy (econet.name, "econet");
	if ((result = register_netdev (&econet))) {
		printk ("econet: Error %d  initializing econet adapter",result);
		return result;
	}
return 0;
}

void econet_cleanup (void)
{
	printk ("<0> Cleaning Up the Module\n");
	unregister_netdev (&econet);
	return;
}

module_init (econet_init_module);
module_exit (econet_cleanup);



static int econet_probe (struct net_device *dev, struct pci_dev *pdev)
{
	int ret;
	unsigned char pci_rev;

	if (! pci_present ()) {
		printk ("No pci device present\n");
		return -ENODEV;
	}
	else  printk ("<0> pci device were found\n");
	
	pdev = pci_find_device (PCI_VENDOR_ID_REALTEK, PCI_DEVICE_ID_REALTEK_8139, pdev);
	
	if (pdev)  printk ("probed for rtl 8139 \n");
	else       printk ("Rtl8193 card not present\n");
	
	pci_read_config_byte (pdev, PCI_REVISION_ID, &pci_rev);
	
	if (ret = pci_enable_device (pdev)) {
		printk ("Error enabling the device\n");
		return ret;
	}
	
	if (pdev->irq < 2) {
		printk ("Invalid irq number\n");
		ret = -EIO;
	}
	else {
		printk ("Irq Obtained is %d",pdev->irq); 
		dev->irq = pdev->irq;
	}
	return 0;
}

int econet_init (struct net_device *dev)
{
	int ret;
	struct pci_dev *pdev = NULL;
	
	if ((ret = econet_probe (dev, pdev)) != 0)
		return ret;
	
	dev->open = econet_open;
	dev->stop = econet_release;
	dev->hard_start_xmit = econet_xmit;
	printk ("Econet adapter initialized\n");
	return 0;
}
