#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "ssd130x.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("junan <kfggww@outlook.com>");
MODULE_DESCRIPTION("Driver of ssd130x oled display.");

struct ssd130x_cdev {
	struct cdev cdev;
	struct i2c_client *client;
	struct class *cls;
	struct ssd130x_type *type;
};

static int ssd130x_cdev_id = 0;

static int ssd130x_open(struct inode *inode, struct file *filp)
{
	struct ssd130x_cdev *dev;
	struct i2c_client *client;
	struct ssd130x_type *t;

	dev = container_of(inode->i_cdev, struct ssd130x_cdev, cdev);
	client = dev->client;
	t = dev->type;

	if (t == NULL || t->init == NULL || client == NULL) {
		pr_err("%s: invalid ssd130x device\n", __func__);
		return -EACCES;
	}

	filp->private_data = dev;
	t->init(dev->client);

	return 0;
}

static int ssd130x_close(struct inode *inocde, struct file *filp)
{
	struct ssd130x_cdev *dev;
	struct i2c_client *client;
	struct ssd130x_type *t;

	dev = filp->private_data;
	client = dev->client;
	t = dev->type;

	if (t == NULL || t->deinit == NULL || client == NULL) {
		pr_err("%s: invalid ssd130x device\n", __func__);
		return -EACCES;
	}

	t->deinit(client);
	return 0;
}

static ssize_t ssd130x_write(struct file *filp, const char __user *buf,
			     size_t n, loff_t *off)
{
	struct ssd130x_cdev *dev;
	struct i2c_client *client;
	struct ssd130x_type *t;

	dev = filp->private_data;
	client = dev->client;
	t = dev->type;

	if (t == NULL || t->write_data_batch == NULL || client == NULL) {
		pr_err("%s: invalid ssd130x device\n", __func__);
		return -EACCES;
	}

	int ret = t->write_data_batch(client, buf, n);
	if (ret)
		return ret;

	return n;
}

static struct file_operations ssd130x_fops = {
	.owner = THIS_MODULE,
	.open = ssd130x_open,
	.release = ssd130x_close,
	.write = ssd130x_write,
};

static int ssd130x_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	/* alloc devno */
	dev_t devno = 0;
	int ret = alloc_chrdev_region(&devno, 0, 1, "ssd130x_cdev");
	if (ret < 0) {
		pr_err("failed to alloc dev number\n");
		goto err1;
	}

	/* alloc ssd130x_cdev */
	struct ssd130x_cdev *dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		pr_err("failed to alloc ssd130x_cdev\n");
		goto err2;
	}

	/* init and add cdev of ssd130x_cdev */
	dev->type = id->driver_data;
	dev->client = client;
	dev_set_drvdata(&client->dev, dev);
	cdev_init(&dev->cdev, &ssd130x_fops);
	ret = cdev_add(&dev->cdev, devno, 1);
	if (ret < 0) {
		pr_err("failed to add cdev\n");
		goto err3;
	}

	/* create device file under "/dev" */
	char class_name[32];
	sprintf(class_name, "ssd130x_cdev_%d", ssd130x_cdev_id);
	dev->cls = class_create(THIS_MODULE, class_name);
	if (dev->cls == NULL) {
		ret = -ENOMEM;
		goto err3;
	}

	struct device *d = device_create(dev->cls, NULL, devno, NULL,
					 "ssd130x%d", ssd130x_cdev_id);
	if (d == NULL) {
		ret = -ENOMEM;
		goto err4;
	}
	ssd130x_cdev_id++;

	return 0;

err4:
	class_destroy(dev->cls);
err3:
	kfree(dev);
err2:
	unregister_chrdev_region(devno, 1);
err1:
	pr_err("%s error\n", __func__);
	return ret;
}

static int ssd130x_remove(struct i2c_client *client)
{
	struct ssd130x_cdev *dev = NULL;
	dev = dev_get_drvdata(&client->dev);

	device_destroy(dev->cls, dev->cdev.dev);
	class_destroy(dev->cls);
	unregister_chrdev_region(dev->cdev.dev, 1);
	kfree(dev);

	return 0;
}

static struct i2c_device_id ssd130x_id_table[] = {
	{ "ssd1306", (kernel_ulong_t)&ssd1306_type },
	{},
};
MODULE_DEVICE_TABLE(i2c, ssd130x_id_table);

static struct i2c_driver ssd130x_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "ssd130x_driver",
    },
	.probe = ssd130x_probe,
    .remove = ssd130x_remove,
	.id_table = ssd130x_id_table,
};

module_i2c_driver(ssd130x_driver);