// SPDX-License-Identifier: GPL-2.0

#include <linux/err.h>
#include <linux/errname.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/kobject.h>
#include <linux/kstrtox.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#define DEFAULT_TIMEOUT_MS 5000

#define BASEPATH "/var/tmp/test_module"
static char filename[NAME_MAX] = "log";

static struct timer_list write_msg_timer;
static atomic_t counter = ATOMIC_INIT(0);

static DEFINE_MUTEX(config_lock);
static unsigned int timeout_ms = DEFAULT_TIMEOUT_MS;

static struct kobject *ksysfs_kobj;

static struct work_struct write_work;

// sysfs filename param
static ssize_t
filename_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t n_bytes;

	mutex_lock(&config_lock);
	n_bytes = scnprintf(buf, PAGE_SIZE, "%s\n", filename);
	mutex_unlock(&config_lock);

	return n_bytes;
}

static ssize_t filename_store(struct kobject *kobj,
			      struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	char temp[NAME_MAX];

	size_t len = strnlen(buf, count);

	while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
		len--;

	if (len == 0 || len >= NAME_MAX)
		return -EINVAL;

	memcpy(temp, buf, len);
	temp[len] = '\0';

	if (strchr(temp, '/'))
		return -EINVAL;

	mutex_lock(&config_lock);
	strscpy(filename, temp, sizeof(filename));
	mutex_unlock(&config_lock);

	return count;
}

static struct kobj_attribute filename_attr =
	__ATTR(filename, 0644, filename_show, filename_store);

// sysfs period param
static ssize_t
period_ms_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned int temp_timeout;

	mutex_lock(&config_lock);
	temp_timeout = timeout_ms;
	mutex_unlock(&config_lock);

	return scnprintf(buf, PAGE_SIZE, "%u\n", temp_timeout);
}

static ssize_t period_ms_store(struct kobject *kobj,
			       struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	unsigned int new_period;
	int error;

	error = kstrtouint(buf, 10, &new_period);
	if (error)
		return error;

	// new_period shouldn't be too small
	if (new_period < 100)
		return -EINVAL;

	mutex_lock(&config_lock);
	timeout_ms = new_period;
	mutex_unlock(&config_lock);

	// reschedule next tick
	mod_timer(&write_msg_timer, jiffies + msecs_to_jiffies(new_period));

	return count;
}

static struct kobj_attribute period_ms_attr =
	__ATTR(period_ms, 0644, period_ms_show, period_ms_store);

static void write_work_func(struct work_struct *work)
{
	char local_name[NAME_MAX];
	char fullpath[PATH_MAX];
	struct file *file;
	loff_t pos = 0;
	ssize_t bytes_written;
	int counter_value;
	char databuf[128];
	int len;

	// concatenate full path
	mutex_lock(&config_lock);
	strscpy(local_name, filename, sizeof(local_name));
	mutex_unlock(&config_lock);

	snprintf(fullpath, sizeof(fullpath), "%s/%s", BASEPATH, local_name);

	// open file
	file = filp_open(fullpath, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (IS_ERR(file)) {
		long error = PTR_ERR(file);

		pr_err("filp_open(%s) failed: %ld (%s). Make sure directory '%s' exists\n",
		       fullpath,
			error,
			errname(error),
			BASEPATH);
		return;
	}

	// get counter value
	counter_value = atomic_inc_return(&counter);

	// concatenate log message
	len = scnprintf(databuf,
			sizeof(databuf),
		"Hello from kernel module (%d)\n",
		counter_value);

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

static void write_msg_timer_callback(struct timer_list *unused)
{
	unsigned int temp_timeout;

	schedule_work(&write_work);

	mutex_lock(&config_lock);
	temp_timeout = timeout_ms;
	mutex_unlock(&config_lock);

	mod_timer(&write_msg_timer, jiffies + msecs_to_jiffies(temp_timeout));
}

// attributes
static struct attribute *ksysfs_attrs[] = {
	&filename_attr.attr,
	&period_ms_attr.attr,
	NULL,
};

static const struct attribute_group ksysfs_attr_group = {
	.attrs = ksysfs_attrs,
};

static int __init ksysfs_logger_init(void)
{
	int ret;

	pr_info("ksysfs logger init\n");

	INIT_WORK(&write_work, write_work_func);
	timer_setup(&write_msg_timer, write_msg_timer_callback, 0);
	mod_timer(&write_msg_timer, jiffies + msecs_to_jiffies(timeout_ms));

	ksysfs_kobj = kobject_create_and_add("ksysfs_logger", kernel_kobj);
	if (!ksysfs_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(ksysfs_kobj, &ksysfs_attr_group);
	if (ret) {
		kobject_put(ksysfs_kobj);
		ksysfs_kobj = NULL;
		return ret;
	}

	return 0;
}

static void __exit ksysfs_logger_exit(void)
{
	pr_info("ksysfs logger exit\n");

	del_timer_sync(&write_msg_timer);
	cancel_work_sync(&write_work);

	if (ksysfs_kobj) {
		sysfs_remove_group(ksysfs_kobj, &ksysfs_attr_group);
		kobject_put(ksysfs_kobj);
		ksysfs_kobj = NULL;
	}
}

module_init(ksysfs_logger_init);
module_exit(ksysfs_logger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mip3x");
MODULE_DESCRIPTION("ksysfs logger");
