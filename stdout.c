#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/tty.h> 
#include <linux/kprobes.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>


#define DEV_NAME "kess-stdout"

MODULE_LICENSE("GPL");


static struct FakeDevice {
    char buffer[10];
    struct semaphore sem;
} virtual_dev;


struct cdev* chdev;
int major_num;
int ret;
dev_t dev_num;

struct task_struct* task;

int dev_open(struct inode* _inode, struct file* filp) {
    // Lock device.
    if (down_interruptible(&virtual_dev.sem) != 0) {
        printk(KERN_CRIT "Could not lock KessStdout dev.\n");
        return -1;
    }

    return 0;
}

static int dev_close(struct inode* _inode, struct file* filp) {
    up(&virtual_dev.sem);
    return 0;
}


static void write(const char* str) {
    struct tty_struct* tty = get_current_tty();
    if (tty) {
        const struct tty_operations* ttyops = tty->driver->ops;
        ttyops->write(tty, str, strlen(str));
        ttyops->write(tty, "\015\012", 2);
    }
}


static ssize_t dev_read(struct file* filp, char* buf, size_t bufcount, loff_t* curOffset) {
    return 0;
}

struct kernel_siginfo info;

static ssize_t dev_write(struct file* filp, const char* bufsrc, size_t bufCount, loff_t* curOffset) {
    ret = copy_from_user(virtual_dev.buffer, bufsrc, bufCount);
    write(virtual_dev.buffer);
    task = current;
    send_sig_info(SIGINT, &info, task);
    return ret;
}


struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_close,
    .write = dev_write,
    .read = dev_read
};


static int __init stdout_init(void) {
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);

    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate a major number.\n");
        return ret;
    }

    major_num = MAJOR(dev_num);

    chdev = cdev_alloc();
    chdev->ops = &fops;
    chdev->owner = THIS_MODULE;
    ret = cdev_add(chdev, dev_num, 1);       // Add cdev to kernel.

    if (ret < 0) {
        printk(KERN_ERR "Failed to add cdev to kernel.\n");
        return ret;
    }

    sema_init(&virtual_dev.sem, 1);

    printk(KERN_INFO "KessStdout major number: %d\n", major_num);
    printk(KERN_INFO "KessStdout Loaded.\n");
    printk(KERN_INFO "Please use: \"mknod /dev/%s c %d 0\"\n", DEV_NAME, major_num);
    return 0;
}


static void __exit stdout_exit(void) {
    cdev_del(chdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "KessStdout Unloaded.\n");
}



module_init(stdout_init);
module_exit(stdout_exit);
