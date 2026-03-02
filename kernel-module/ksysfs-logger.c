#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/timer.h>
#include <linux/types.h>

#define TIMEOUT_MS 5000

static struct timer_list write_msg_timer;
static atomic_t counter = ATOMIC_INIT(0);

static void write_msg_timer_callback(struct timer_list *unused) {
    int counter_value = atomic_inc_return(&counter);
    pr_info("Hello from kernel module (%d)\n", counter_value);

    mod_timer(&write_msg_timer, jiffies + msecs_to_jiffies(TIMEOUT_MS));
}

static int __init ksysfs_logger_init(void) {
    pr_info("ksysfs logger init\n");

    timer_setup(&write_msg_timer, write_msg_timer_callback, 0);
    mod_timer(&write_msg_timer, jiffies + msecs_to_jiffies(TIMEOUT_MS));

    return 0;
}

static void __exit ksysfs_logger_exit(void) {
    pr_info("ksysfs logger exit\n");

    del_timer_sync(&write_msg_timer);
}

module_init(ksysfs_logger_init);
module_exit(ksysfs_logger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mip3x");
MODULE_DESCRIPTION("ksysfs logger");
