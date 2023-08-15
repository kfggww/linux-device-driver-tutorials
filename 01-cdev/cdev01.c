#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("junan <kfggww@outlook.com>");
MODULE_DESCRIPTION(
	"cdev01: Basic char device, write(read) data to(from) a ring buffer in kernel module.");

static struct cdev01 {
	struct cdev cdev;
	void *rbuf;
	int read_ptr;
	int write_ptr;
	int size;
	int cap;
} cdev01;

static ssize_t cdev01_read(struct file *fp, char __user *buf, size_t n,
			   loff_t *off)
{
	ssize_t total_read = 0;
	int nread = 0;

	while (cdev01.size > 0 && n > 0) {
		if (cdev01.read_ptr < cdev01.write_ptr) {
			nread = n < cdev01.write_ptr - cdev01.read_ptr ?
					n :
					cdev01.write_ptr - cdev01.read_ptr;
		} else {
			nread = n < cdev01.cap - cdev01.read_ptr ?
					n :
					cdev01.cap - cdev01.read_ptr;
		}

		copy_to_user(buf, cdev01.rbuf + cdev01.read_ptr, nread);
		total_read += nread;
		n -= nread;
		cdev01.size -= nread;
		cdev01.read_ptr = (cdev01.read_ptr + nread) % cdev01.cap;
	}

	*off += total_read;
	return total_read;
}

static ssize_t cdev01_write(struct file *fp, const char __user *buf, size_t n,
			    loff_t *off)
{
	ssize_t total_write = 0;
	int nwrite = 0;

	/* just return an error if user space try to write and the ring buffer is full */
	if (cdev01.cap == cdev01.size && n > 0)
		return -EBUSY;

	while (cdev01.size < cdev01.cap && n > 0) {
		if (cdev01.write_ptr >= cdev01.read_ptr) {
			nwrite = n < cdev01.cap - cdev01.write_ptr ?
					 n :
					 cdev01.cap - cdev01.write_ptr;
		} else {
			nwrite = n < cdev01.read_ptr - cdev01.write_ptr ?
					 n :
					 cdev01.read_ptr - cdev01.write_ptr;
		}

		copy_from_user(cdev01.rbuf + cdev01.write_ptr, buf, nwrite);
		total_write += nwrite;
		n -= nwrite;
		cdev01.size += nwrite;
		cdev01.write_ptr = (cdev01.write_ptr + nwrite) % cdev01.cap;
	}

	*off += total_write;
	return total_write;
}

static int rbuf_max_size = 1024;

static struct file_operations cdev01_fops = {
	.owner = THIS_MODULE,
	.read = cdev01_read,
	.write = cdev01_write,
};

module_param(rbuf_max_size, int, 0644);

static int __init cdev01_init(void)
{
	int ret = 0;

	/* alloc memory for ring buffer */
	cdev01.size = 0;
	cdev01.read_ptr = 0;
	cdev01.write_ptr = 0;
	cdev01.cap = rbuf_max_size;

	cdev01.rbuf = kmalloc(rbuf_max_size, GFP_KERNEL);
	if (cdev01.rbuf == NULL) {
		ret = -ENOMEM;
		goto err1;
	}

	/* alloc char dev number */
	dev_t devno = 0;
	ret = alloc_chrdev_region(&devno, 0, 1, "cdev01");
	if (ret) {
		goto err2;
	}
	pr_info("cdev01: major=%d, minor=%d\n", MAJOR(devno), MINOR(devno));

	/* init and add char dev to the kernel */
	cdev_init(&cdev01.cdev, &cdev01_fops);
	ret = cdev_add(&cdev01.cdev, devno, 1);
	if (ret) {
		goto err3;
	}

	pr_info("%s: done\n", __func__);
	return 0;

err3:
	unregister_chrdev_region(devno, 1);
err2:
	kfree(cdev01.rbuf);
err1:
	pr_err("%s: error\n", __func__);
	return ret;
};

static void __exit cdev01_exit(void)
{
	if (cdev01.rbuf != NULL) {
		kfree(cdev01.rbuf);
		cdev01.rbuf = NULL;
	}

	unregister_chrdev_region(cdev01.cdev.dev, 1);
	cdev_del(&cdev01.cdev);

	pr_info("%s: done\n", __func__);
}

module_init(cdev01_init);
module_exit(cdev01_exit);