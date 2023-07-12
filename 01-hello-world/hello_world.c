#include <linux/module.h>

static int __init hello_world_init(void)
{
	pr_info("hello_world module init\n");
	return 0;
}

static void __exit hello_world_exit(void)
{
	pr_info("hello_world module exit\n");
}

module_init(hello_world_init);
module_exit(hello_world_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("junan");
MODULE_DESCRIPTION("LDDT: A simple linux kernel module");