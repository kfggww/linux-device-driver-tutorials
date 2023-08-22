#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/poll.h>

#define MYDEVICE_NAME "cdev03"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("junan <kfggww@outlook.com>");
MODULE_DESCRIPTION("cdev03: block and non-block IO");

struct mycdev {
	struct cdev cdev;
	struct class *cls;
	struct fasync_struct *fa;
	char *buf;
	char *buf_end;
	char *rptr;
	char *wptr;
	int buf_size;
};

static struct mycdev *cdev03 = NULL;
static int rbuf_max_size = 1024;
module_param(rbuf_max_size, int, 0644);

DECLARE_WAIT_QUEUE_HEAD(read_queue);
DECLARE_WAIT_QUEUE_HEAD(write_queue);
DEFINE_MUTEX(cdev03_lock);

static int space_used(void)
{
	if (cdev03->rptr <= cdev03->wptr) {
		return cdev03->wptr - cdev03->rptr;
	} else {
		return cdev03->buf_size + cdev03->wptr - cdev03->rptr;
	}
}

static int space_free(void)
{
	if (cdev03->rptr <= cdev03->wptr) {
		return cdev03->buf_size + cdev03->rptr - cdev03->wptr - 1;
	} else {
		return cdev03->rptr - cdev03->wptr - 1;
	}
}

static ssize_t cdev03_read(struct file *filp, char __user *ubuf, size_t n,
			   loff_t *off)
{
	if (mutex_lock_interruptible(&cdev03_lock))
		return -ERESTARTSYS;

	while (space_used() == 0) {
		mutex_unlock(&cdev03_lock);

		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		} else {
			if (wait_event_interruptible(read_queue,
						     space_used() > 0))
				return -ERESTARTSYS;
		}

		if (mutex_lock_interruptible(&cdev03_lock))
			return -ERESTARTSYS;
	}

	size_t nread = min(n, space_used());

	if (cdev03->rptr < cdev03->wptr) {
		nread = min(nread, (size_t)(cdev03->wptr - cdev03->rptr));
	} else {
		nread = min(nread, (size_t)(cdev03->buf_end - cdev03->rptr));
	}

	nread -= copy_to_user(ubuf, cdev03->rptr, nread);
	cdev03->rptr += nread;
	if (cdev03->rptr == cdev03->buf_end)
		cdev03->rptr = cdev03->buf;

	if (space_free() > 0) {
		wake_up_interruptible(&write_queue);
	}

	mutex_unlock(&cdev03_lock);

	*off += nread;
	return nread;
}

static ssize_t cdev03_write(struct file *filp, const char __user *ubuf,
			    size_t n, loff_t *off)
{
	if (mutex_lock_interruptible(&cdev03_lock))
		return -ERESTARTSYS;

	while (space_free() == 0) {
		mutex_unlock(&cdev03_lock);

		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		} else {
			if (wait_event_interruptible(write_queue,
						     space_free() > 0))
				return -ERESTARTSYS;
		}

		if (mutex_lock_interruptible(&cdev03_lock))
			return -ERESTARTSYS;
	}

	size_t nwrite = min(n, space_free());

	if (cdev03->rptr <= cdev03->wptr) {
		nwrite = min(nwrite, (size_t)(cdev03->buf_end - cdev03->wptr));
	} else {
		nwrite = min(nwrite, (size_t)(cdev03->rptr - cdev03->wptr - 1));
	}

	nwrite -= copy_from_user(cdev03->wptr, ubuf, nwrite);
	cdev03->wptr += nwrite;
	if (cdev03->wptr == cdev03->buf_end)
		cdev03->wptr = cdev03->buf;

	if (space_used() > 0) {
		wake_up_interruptible(&read_queue);
		kill_fasync(&cdev03->fa, SIGIO, POLL_IN);
	}

	mutex_unlock(&cdev03_lock);

	*off += nwrite;
	return nwrite;
}

static __poll_t cdev03_poll(struct file *filp,
			    struct poll_table_struct *poll_table)
{
	__poll_t mask = 0;

	if (mutex_lock_interruptible(&cdev03_lock))
		return -ERESTARTSYS;

	poll_wait(filp, &read_queue, poll_table);
	poll_wait(filp, &write_queue, poll_table);

	if (space_used() > 0) {
		mask |= POLLIN | POLLRDNORM;
	}
	if (space_free() > 0) {
		mask |= POLLOUT | POLLWRNORM;
	}

	mutex_unlock(&cdev03_lock);

	return mask;
}

static int cdev03_fasync(int fd, struct file *filp, int on)
{
	return fasync_helper(fd, filp, on, &cdev03->fa);
}

static int cdev03_close(struct inode *inode, struct file *filp)
{
	return fasync_helper(-1, filp, 0, &cdev03->fa);
}

static struct file_operations cdev03_fops = {
	.read = cdev03_read,
	.write = cdev03_write,
	.poll = cdev03_poll,
	.fasync = cdev03_fasync,
	.release = cdev03_close,
};

static int __init cdev03_init(void)
{
	int ret = 0;

	/* init cdev03 */
	if (cdev03 == NULL) {
		cdev03 = kmalloc(sizeof(*cdev03), GFP_KERNEL);
		if (cdev03 == NULL) {
			ret = -ENOMEM;
			goto err1;
		}

		if (rbuf_max_size <= 0) {
			ret = -EINVAL;
			goto err2;
		}

		cdev03->fa = NULL;

		cdev03->buf = kmalloc(rbuf_max_size, GFP_KERNEL);
		if (cdev03->buf == NULL) {
			ret = -ENOMEM;
			goto err2;
		}

		cdev03->buf_end = cdev03->buf + rbuf_max_size;
		cdev03->rptr = cdev03->buf;
		cdev03->wptr = cdev03->buf;
		cdev03->buf_size = rbuf_max_size;
	}

	/* init cdev, add it to the system */
	dev_t devno = 0;
	ret = alloc_chrdev_region(&devno, 0, 1, MYDEVICE_NAME);
	if (ret) {
		goto err3;
	}

	cdev_init(&cdev03->cdev, &cdev03_fops);
	ret = cdev_add(&cdev03->cdev, devno, 1);
	if (ret) {
		goto err4;
	}

	/* ignore checking the return value of these two APIs */
	cdev03->cls = class_create(THIS_MODULE, "mycdev");
	device_create(cdev03->cls, NULL, devno, NULL, "%s", MYDEVICE_NAME);

	pr_info("cdev03: major=%d, minor=%d\n", MAJOR(devno), MINOR(devno));
	pr_info("%s: done\n", __func__);
	return 0;

err4:
	unregister_chrdev_region(devno, 1);
err3:
	kfree(cdev03->buf);
err2:
	kfree(cdev03);
err1:
	return ret;
}

static void __exit cdev03_exit(void)
{
	if (cdev03 == NULL)
		goto done;

	device_destroy(cdev03->cls, cdev03->cdev.dev);
	class_destroy(cdev03->cls);
	unregister_chrdev_region(cdev03->cdev.dev, 1);
	kfree(cdev03->buf);
	kfree(cdev03);

done:
	pr_info("%s: done\n", __func__);
}

module_init(cdev03_init);
module_exit(cdev03_exit);