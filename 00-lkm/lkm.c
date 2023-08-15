#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("junan <kfggww@outlook.com>");
MODULE_DESCRIPTION("Linux kernel module demo.");

static int __init lkm_init(void)
{
	pr_info("%s done\n", __func__);
	return 0;
}

static void __exit lkm_exit(void)
{
	pr_info("%s done\n", __func__);
}

module_init(lkm_init);
module_exit(lkm_exit);