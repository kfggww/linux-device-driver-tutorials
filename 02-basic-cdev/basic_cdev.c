#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>

struct cdev my_cdev;
dev_t dev_num;

static ssize_t basic_read(struct file *fp, char __user *buf, size_t size,
			  loff_t *offset);

struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = basic_read,
};

static ssize_t basic_read(struct file *fp, char __user *buf, size_t size,
			  loff_t *offset)
{
	pr_info("basic_cdev: read\n");
	return 0;
}

static int __init basic_cdev_init(void)
{
	int err = alloc_chrdev_region(&dev_num, 0, 1, "basic_cdev");
	if (err) {
		pr_err("%s: alloc dev_t fail\n", __func__);
		goto err_dev_t;
	}
	pr_info("%s: major=%d, minor=%d\n", __func__, MAJOR(dev_num),
		MINOR(dev_num));

	cdev_init(&my_cdev, &fops);
	err = cdev_add(&my_cdev, dev_num, 1);
	if (err) {
		pr_err("%s: add cdev fail\n", __func__);
		goto err_add_cdev;
	}

	pr_info("%s: done\n", __func__);

	return 0;

err_add_cdev:
	unregister_chrdev_region(dev_num, 1);
err_dev_t:
	return -1;
}

static void __exit basic_cdev_exit(void)
{
	unregister_chrdev_region(dev_num, 1);
	cdev_del(&my_cdev);
	pr_info("%s: done\n", __func__);
}

module_init(basic_cdev_init);
module_exit(basic_cdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("junan");
MODULE_DESCRIPTION("basic char device deriver.");