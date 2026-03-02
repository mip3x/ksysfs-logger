#include <linux/err.h>
#include <linux/errname.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#define TIMEOUT_MS 5000

#define BASEPATH "/var/tmp/test_module"
static char filename[NAME_MAX] = "log";

static struct timer_list write_msg_timer;
static atomic_t counter = ATOMIC_INIT(0);

static struct work_struct write_work;

static void write_work_func(struct work_struct *work) {
    char fullpath[PATH_MAX];
    struct file *file;
    loff_t pos = 0;
    ssize_t bytes_written;
    int counter_value;
    char databuf[128];
    int len;

    // concatenate full path
    snprintf(fullpath, sizeof(fullpath), "%s/%s", BASEPATH, filename);

    // open file
    file = filp_open(fullpath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (IS_ERR(file)) {
        long error = PTR_ERR(file);
        pr_err(
            "filp_open(%s) failed: %ld (%s). Make sure directory '%s' exists\n",
            fullpath,
            error,
            errname(error),
            BASEPATH
        );
        return;
    }

    // get counter value
    counter_value = atomic_inc_return(&counter);

    // concatenate log message
    len = scnprintf(databuf, sizeof(databuf), "Hello from kernel module (%d)\n", counter_value);
    pr_info("%s", databuf);

    // write file
    bytes_written = kernel_write(file, databuf, len, &pos);
    if (bytes_written < 0) {
        pr_err("kernel_write failed: %zd\n", bytes_written);
        filp_close(file, NULL);
        return;
    }

    // close file
    filp_close(file, NULL);
}

static void write_msg_timer_callback(struct timer_list *unused) {
    schedule_work(&write_work);
    mod_timer(&write_msg_timer, jiffies + msecs_to_jiffies(TIMEOUT_MS));
}

static int __init ksysfs_logger_init(void) {
    pr_info("ksysfs logger init\n");

    INIT_WORK(&write_work, write_work_func);
    timer_setup(&write_msg_timer, write_msg_timer_callback, 0);
    mod_timer(&write_msg_timer, jiffies + msecs_to_jiffies(TIMEOUT_MS));

    return 0;
}

static void __exit ksysfs_logger_exit(void) {
    pr_info("ksysfs logger exit\n");

    del_timer_sync(&write_msg_timer);
    cancel_work_sync(&write_work);
}

module_init(ksysfs_logger_init);
module_exit(ksysfs_logger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mip3x");
MODULE_DESCRIPTION("ksysfs logger");
