#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/syscore_ops.h>
#include <linux/irq.h>
#include <linux/fcntl.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>	
#include "bcm2835.h"

#define DHT11_DEV_NAME "dht11"
#define DHT11_MAJOR_NUMBER 507
#define DHT11_IOCTL_MAGIC_NUMBER 	'x'
#define DHT11_IOCTL_READ_TEMP 		_IOWR(DHT11_IOCTL_MAGIC_NUMBER, 0, int)
#define DHT11_SIGNAL_PIN 16

#define DHT11_TIMEOUT		0xFFFF
#define DHT11_INVALID_CHECKSUM  0xFFFE
#define DHT11_OK		0x0

volatile unsigned int *gpio_base, *gpio_gpset0;
int read_sensor(uint8_t *bits);
int check_checksum(uint8_t *bits);

int dev_open(struct inode *inode, struct file *filep) {
	printk(KERN_ALERT "[%s] driver open \n", DHT11_DEV_NAME);
	gpio_base = (uint32_t *)ioremap(GPIO_BASE_ADDR, 0x60);		
	return 0;
}

int dev_release(struct inode *inode, struct file *filep) {
	printk(KERN_ALERT "[%s] driver closed\n", DHT11_DEV_NAME);
	iounmap((void *)gpio_base);
	return 0;
}

long dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {	
	int retval;
	int32_t temperature = 0;

	uint8_t bits[5];
	switch(cmd) {
		case DHT11_IOCTL_READ_TEMP:
		       
			retval = read_sensor(bits);
			if (retval != DHT11_OK) {
				retval = -1;
				printk(KERN_ALERT "dht11 read failed");
				copy_to_user((const void*)arg, &retval, 4);
				break;
			}
			retval = check_checksum(bits);
			printk(KERN_ALERT "checksum correct? : %d\n", retval);
			temperature = bits[2];
			copy_to_user((const void*)arg, &temperature, 4);
			break;
	}
	return 0;

}
int read_sensor(uint8_t *bits){
	int ret = DHT11_OK;
	uint8_t mask = 128;
	uint8_t idx = 0;
	int i;
	uint16_t loop_cnt_low = 100;
	uint16_t loop_cnt_high = 100;
	uint8_t bit_val = 0;
	int retval;
	pinModeAlt(gpio_base, DHT11_SIGNAL_PIN, FSEL_OUTP);

	/** empty buffer */
	for (i=0; i<5; i++) bits[i] = 0;
	
			
	digitalWrite(gpio_base, DHT11_SIGNAL_PIN, LOW); //low pin
	mdelay(18);
	digitalWrite(gpio_base, DHT11_SIGNAL_PIN, HIGH);	//set pin high to collect hum&temp
	udelay(30);			
	pinModeAlt(gpio_base, DHT11_SIGNAL_PIN, FSEL_INPT);
	
	for (i=40; i != 0; --i){
		loop_cnt_low = 100;
		bit_val = gpio_get_value(DHT11_SIGNAL_PIN);
		while(bit_val == 0)
		{	
			bit_val = gpio_get_value(DHT11_SIGNAL_PIN);
			--loop_cnt_low;	
			if (loop_cnt_low == 0) return DHT11_TIMEOUT;
			udelay(1);
		}
		printk(KERN_ALERT "1\n");	
		loop_cnt_high = 100;
		bit_val = gpio_get_value(DHT11_SIGNAL_PIN);
		while(bit_val != 0)
		{	
			bit_val = gpio_get_value(DHT11_SIGNAL_PIN);
			--loop_cnt_high;	
			if (loop_cnt_high == 0) return DHT11_TIMEOUT;
			udelay(1);
		}
		printk(KERN_ALERT "2\n");	
		if ((100 - loop_cnt_high) > 30) bits[idx] |= mask;
		mask >>= 1;
		if (mask == 0)
		{
			mask = 128;
			idx++;
		}
	}
	return ret;
}
 int check_checksum(uint8_t *bits){
	uint8_t sum = bits[0] + bits[1] + bits[2] + bits[3];
	if (bits[4] != sum)
	{
		return DHT11_INVALID_CHECKSUM;
	}

	return DHT11_OK;
}

static struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.unlocked_ioctl = dev_ioctl	
};

int __init dev_init(void) {
	if(register_chrdev(DHT11_MAJOR_NUMBER, DHT11_DEV_NAME, &dev_fops) < 0)
		printk(KERN_ALERT "[%s] driver init failed\n");
	else
		printk(KERN_ALERT "[%s] driver init successful\n");


	return 0;
}

void __exit dev_exit(void) {
	printk(KERN_ALERT, "[%s] driver cleanup\n", DHT11_DEV_NAME);
	unregister_chrdev(DHT11_MAJOR_NUMBER, DHT11_DEV_NAME);	
}

module_init(dev_init);
module_exit(dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("System Programming Final Project - Humidity& Sensor");
