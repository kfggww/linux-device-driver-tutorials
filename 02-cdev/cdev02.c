#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define MYDEVICE_NAME "cdev02"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("junan <kfggww@outlook.com>");
MODULE_DESCRIPTION("cdev02: block and non-block IO");

struct mycdev {
	struct cdev cdev;
	struct class *cls;
	char *buf;
	char *buf_end;
	char *rptr;
	char *wptr;
	int buf_size;
};

static struct mycdev *cdev02 = NULL;
static int rbuf_max_size = 1024;
module_param(rbuf_max_size, int, 0644);

DECLARE_WAIT_QUEUE_HEAD(read_queue);
DECLARE_WAIT_QUEUE_HEAD(write_queue);
DEFINE_MUTEX(cdev02_lock);

static int space_used(void)
{
	if (cdev02->rptr <= cdev02->wptr) {
		return cdev02->wptr - cdev02->rptr;
	} else {
		return cdev02->buf_size + cdev02->wptr - cdev02->rptr;
	}
}

static int space_free(void)
{
	if (cdev02->rptr <= cdev02->wptr) {
		return cdev02->buf_size + cdev02->rptr - cdev02->wptr - 1;
	} else {
		return cdev02->rptr - cdev02->wptr - 1;
	}
}

static ssize_t cdev02_read(struct file *filp, char __user *ubuf, size_t n,
			   loff_t *off)
{
	if (mutex_lock_interruptible(&cdev02_lock))
		return -ERESTARTSYS;

	while (space_used() == 0) {
		mutex_unlock(&cdev02_lock);

		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		} else {
			if (wait_event_interruptible(read_queue,
						     space_used() > 0))
				return -ERESTARTSYS;
		}

		if (mutex_lock_interruptible(&cdev02_lock))
			return -ERESTARTSYS;
	}

	size_t nread = min(n, space_used());

	if (cdev02->rptr < cdev02->wptr) {
		nread = min(nread, (size_t)(cdev02->wptr - cdev02->rptr));
	} else {
		nread = min(nread, (size_t)(cdev02->buf_end - cdev02->rptr));
	}

	nread -= copy_to_user(ubuf, cdev02->rptr, nread);
	cdev02->rptr += nread;
	if (cdev02->rptr == cdev02->buf_end)
		cdev02->rptr = cdev02->buf;

	if (space_free() > 0) {
		wake_up_interruptible(&write_queue);
	}

	mutex_unlock(&cdev02_lock);

	*off += nread;
	return nread;
}

static ssize_t cdev02_write(struct file *filp, const char __user *ubuf,
			    size_t n, loff_t *off)
{
	if (mutex_lock_interruptible(&cdev02_lock))
		return -ERESTARTSYS;

	while (space_free() == 0) {
		mutex_unlock(&cdev02_lock);

		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		} else {
			if (wait_event_interruptible(write_queue,
						     space_free() > 0))
				return -ERESTARTSYS;
		}

		if (mutex_lock_interruptible(&cdev02_lock))
			return -ERESTARTSYS;
	}

	size_t nwrite = min(n, space_free());

	if (cdev02->rptr <= cdev02->wptr) {
		nwrite = min(nwrite, (size_t)(cdev02->buf_end - cdev02->wptr));
	} else {
		nwrite = min(nwrite, (size_t)(cdev02->rptr - cdev02->wptr - 1));
	}

	nwrite -= copy_from_user(cdev02->wptr, ubuf, nwrite);
	cdev02->wptr += nwrite;
	if (cdev02->wptr == cdev02->buf_end)
		cdev02->wptr = cdev02->buf;

	if (space_used() > 0) {
		wake_up_interruptible(&read_queue);
	}

	mutex_unlock(&cdev02_lock);

	*off += nwrite;
	return nwrite;
}

static struct file_operations cdev02_fops = {
	.read = cdev02_read,
	.write = cdev02_write,
};

static int __init cdev02_init(void)
{
	int ret = 0;

	/* init cdev02 */
	if (cdev02 == NULL) {
		cdev02 = kmalloc(sizeof(*cdev02), GFP_KERNEL);
		if (cdev02 == NULL) {
			ret = -ENOMEM;
			goto err1;
		}

		if (rbuf_max_size <= 0) {
			ret = -EINVAL;
			goto err2;
		}

		cdev02->buf = kmalloc(rbuf_max_size, GFP_KERNEL);
		if (cdev02->buf == NULL) {
			ret = -ENOMEM;
			goto err2;
		}

		cdev02->buf_end = cdev02->buf + rbuf_max_size;
		cdev02->rptr = cdev02->buf;
		cdev02->wptr = cdev02->buf;
		cdev02->buf_size = rbuf_max_size;
	}

	/* init cdev, add it to the system */
	dev_t devno = 0;
	ret = alloc_chrdev_region(&devno, 0, 1, MYDEVICE_NAME);
	if (ret) {
		goto err3;
	}

	cdev_init(&cdev02->cdev, &cdev02_fops);
	ret = cdev_add(&cdev02->cdev, devno, 1);
	if (ret) {
		goto err4;
	}

	/* ignore checking the return value of these two APIs */
	cdev02->cls = class_create(THIS_MODULE, "mycdev");
	device_create(cdev02->cls, NULL, devno, NULL, "%s", MYDEVICE_NAME);

	pr_info("cdev02: major=%d, minor=%d\n", MAJOR(devno), MINOR(devno));
	pr_info("%s: done\n", __func__);
	return 0;

err4:
	unregister_chrdev_region(devno, 1);
err3:
	kfree(cdev02->buf);
err2:
	kfree(cdev02);
err1:
	return ret;
}

static void __exit cdev02_exit(void)
{
	if (cdev02 == NULL)
		goto done;

	device_destroy(cdev02->cls, cdev02->cdev.dev);
	class_destroy(cdev02->cls);
	unregister_chrdev_region(cdev02->cdev.dev, 1);
	kfree(cdev02->buf);
	kfree(cdev02);

done:
	pr_info("%s: done\n", __func__);
}

module_init(cdev02_init);
module_exit(cdev02_exit);