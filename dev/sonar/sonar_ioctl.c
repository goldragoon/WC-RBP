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

#include "bcm2835.h"

#define SONAR_MAJOR_NUMBER 505
#define SONAR_DEV_NAME "sonar_ioctl"
#define SONAR_IOCTL_MAGIC_NUMBER 	'o'
#define SONAR_IOCTL_READ_DIST 		_IOWR(SONAR_IOCTL_MAGIC_NUMBER, 0, int)
#define SONAR_IOCTL_SET_TARGET 		_IOWR(SONAR_IOCTL_MAGIC_NUMBER, 1, int)
#define SENSOR_COUNT 3

int GPIO_ECHO[SENSOR_COUNT] = {23, 20, 19};
int GPIO_TRIG[SENSOR_COUNT] = {24, 21, 26};

int i;
int sensor_num = 0;	
volatile unsigned int *gpio_base, *gpio_gpset0;

int dev_open(struct inode *inode, struct file *filep) {
	printk(KERN_ALERT "[%s] driver open \n", SONAR_DEV_NAME);

	gpio_base = (uint32_t *)ioremap(GPIO_BASE_ADDR, 0x60);
	for(i = 0; i < SENSOR_COUNT; i++) {
		pinModeAlt(gpio_base, GPIO_ECHO[i], FSEL_INPT);
		pinModeAlt(gpio_base, GPIO_TRIG[i], FSEL_OUTP);
	}
	return 0;
}

int dev_release(struct inode *inode, struct file *filep) {
	printk(KERN_ALERT "[%s] driver closed\n", SONAR_DEV_NAME);
	iounmap((void *)gpio_base);
	return 0;
}


long dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

	
	unsigned int udelayed_high = 0, udelayed_low;
	switch(cmd) {
		case SONAR_IOCTL_SET_TARGET:
			copy_from_user(&sensor_num, (const void*)arg, 4);
			break;

		case SONAR_IOCTL_READ_DIST:
			//printk(KERN_ALERT "SONAR_READ_DIST : %d\n", sensor_num);
			digitalWrite(gpio_base, GPIO_TRIG[sensor_num], HIGH);
			udelay(10);
			digitalWrite(gpio_base, GPIO_TRIG[sensor_num], LOW);
	
			udelayed_low = 0;	
			while(digitalRead(gpio_base, GPIO_ECHO[sensor_num]) == LOW) { 
				udelayed_low +=2; 
				udelay(2);
				if(udelayed_low > 10000) break;
			}

			udelayed_high = 0;			
			while(digitalRead(gpio_base, GPIO_ECHO[sensor_num]) == HIGH) {udelayed_high += 2; udelay(2); }	
			copy_to_user((const void*)arg, &udelayed_high, 4);
			break;	
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
	if(register_chrdev(SONAR_MAJOR_NUMBER, SONAR_DEV_NAME, &dev_fops) < 0)
		printk(KERN_ALERT "[%s] driver init failed\n");
	else
		printk(KERN_ALERT "[%s] driver init successful\n");


	return 0;
}

void __exit dev_exit(void) {
	printk(KERN_ALERT, "[%s] driver cleanup\n", SONAR_DEV_NAME);
	unregister_chrdev(SONAR_MAJOR_NUMBER, SONAR_DEV_NAME);	
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gyu Jin Choi");
MODULE_DESCRIPTION("System Programming Final Project - Sonar Sensor");
