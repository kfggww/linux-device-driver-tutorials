#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "commands.h"

#define SSD130x_DEVICE_NAME "ssd1306_disp"
#define SSD130x_DRIVER_NAME "ssd130x_disp_driver"

struct ssd130x_device {
	struct i2c_client *client;
	struct cdev cdev;
	struct list_head entry;
};

static LIST_HEAD(ssd130x_devs);

static int ssd130x_open(struct inode *inode, struct file *fp)
{
	struct list_head *pos;
	list_for_each (pos, &ssd130x_devs) {
		struct ssd130x_device *dev =
			container_of(pos, struct ssd130x_device, entry);

		if (inode->i_cdev == &dev->cdev) {
			pr_info("%s: found cdev\n", __func__);
			fp->private_data = dev;
			break;
		}
	}

	if (fp->private_data == NULL) {
		pr_info("%s: no cdev found\n", __func__);
		return -1;
	}

	return 0;
}

static ssize_t ssd130x_write(struct file *fp, const char __user *buf, size_t n,
			     loff_t *off)
{
	pr_info("%s: called\n", __func__);
	struct ssd130x_device *dev = fp->private_data;
	char *data = kmalloc(n, GFP_KERNEL);
	copy_from_user(data, buf, n);

	ssd130x_disp_print(dev->client, data);

	kfree(data);
	return n;
}

static struct file_operations ssd130x_fops = {
	.open = ssd130x_open,
	.write = ssd130x_write,
};

int ssd130x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	pr_info("new i2c device detected: addr=%x\n", client->addr);

	struct ssd130x_device *dev =
		kmalloc(sizeof(struct ssd130x_device), GFP_KERNEL);
	dev->client = client;
	INIT_LIST_HEAD(&dev->entry);
	list_add_tail(&dev->entry, &ssd130x_devs);

	dev_t dev_num;
	alloc_chrdev_region(&dev_num, 0, 1, "ssd130x-disp");
	cdev_init(&dev->cdev, &ssd130x_fops);
	cdev_add(&dev->cdev, dev_num, 1);

	pr_info("new cdev: major=%d, minor=%d\n", MAJOR(dev_num),
		MINOR(dev_num));

	ssd130x_disp_init(client);

	return 0;
}

int ssd130x_remove(struct i2c_client *client)
{
	pr_info("i2c device removed: addr=%x\n", client->addr);
	return 0;
}

/* device id supported */
static struct i2c_device_id ssd130x_device_id[] = {
	{ SSD130x_DEVICE_NAME, 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, ssd130x_device_id);

static struct i2c_driver ssd130x_disp_driver = {
	.driver = {
		.name = SSD130x_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
	.probe = ssd130x_probe,
	.remove = ssd130x_remove,
	.id_table = ssd130x_device_id,
};

static int __init ssd130x_init(void)
{
	i2c_add_driver(&ssd130x_disp_driver);
	pr_info("%s: init\n", __func__);
	return 0;
}

static void __exit ssd130x_exit(void)
{
	i2c_del_driver(&ssd130x_disp_driver);

	struct list_head *pos, *n;
	list_for_each_safe (pos, n, &ssd130x_devs) {
		struct ssd130x_device *dev =
			container_of(pos, struct ssd130x_device, entry);
		unregister_chrdev_region(dev->cdev.dev, dev->cdev.count);
		cdev_del(&dev->cdev);
		list_del(pos);
		kfree(dev);
	}

	pr_info("%s: exit\n", __func__);
}

module_init(ssd130x_init);
module_exit(ssd130x_exit);