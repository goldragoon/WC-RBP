#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/unistd.h>

#include <linux/delay.h>
#include <linux/timer.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define TOUCH_MAJOR_NUMBER 501
#define TOUCH_DEV_NAME "touch_ioctl"
#define TOUCH_IOCTL_MAGIC_NUMBER 	't'
#define TOUCH_IOCTL_IS_TOUCHED 		_IOWR(TOUCH_IOCTL_MAGIC_NUMBER, 0, int)

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0x04
#define GPLEV0 0x34

static void __iomem *gpio_base;
volatile unsigned int *gpsel2;
volatile unsigned int *gplev0;
int dev_open(struct inode *inode, struct file *filep) {
	printk(KERN_ALERT "[%s] driver open \n", TOUCH_DEV_NAME);

	gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
	gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
	gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
	return 0;
}


int dev_release(struct inode *inode, struct file *filep) {
	printk(KERN_ALERT, "[%s] driver closed\n", TOUCH_DEVNAME);
	iounmap((void *)gpio_base);
	return 0;
}

long dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	
	switch(cmd) {
		
	}

	return 0;
}

static struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.unlocked_ioctl = dev_ioctl	
};

int __init dev_init(void) {
	if(register_chrdev(TOUCH_MAJOR_NUMBER, TOUCH_DEV_NAME, &dev_fops) < 0)
		printk(KERN_ALERT "[%s] driver init failed\n");
	else
		printk(KERN_ALERT "[%s] driver init successful\n");


	return 0;
}

void __exit dev_exit(void) {
	printk(KERN_ALERT, "[%s] driver cleanup\n", TOUCH_DEV_NAME);
	unregister_chrdev(TOUCH_MAJOR_NUMBER, TOUCH_DEV_NAME);	
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gyu Jin Choi");
MODULE_DESCRIPTION("System Programming Final Project - Touch Sensor");
