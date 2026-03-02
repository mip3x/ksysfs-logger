#include <linux/module.h>
#include <linux/printk.h>

static int __init sysfs_logger_init(void) {
    pr_info("ksysfs logger init\n");
    return 0;
}

static void __exit sysfs_logger_exit(void) {
    pr_info("ksysfs logger exit\n");
}

module_init(sysfs_logger_init);
module_exit(sysfs_logger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mip3x");
MODULE_DESCRIPTION("ksysfs logger");
