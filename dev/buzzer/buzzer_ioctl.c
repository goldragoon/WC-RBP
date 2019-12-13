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

#define BUZZER_MAJOR_NUMBER 502
#define BUZZER_DEV_NAME "buzzer_ioctl"
#define BUZZER_IOCTL_MAGIC_NUMBER 'p'
#define BUZZER_IOCTL_CMD_PLAY 		_IOWR(BUZZER_IOCTL_MAGIC_NUMBER, 0, int)
#define BUZZER_IOCTL_CMD_STOP		_IOWR(BUZZER_IOCTL_MAGIC_NUMBER, 1, int)
#define GPIO_BUZZER 13

#define GPIO_BASE_ADDR 0x3F200000
#define PWM_BASE_ADDR 0x3F20C000
#define CLK_BASE_ADDR 0x3F101000

// GPIO Control Related
volatile unsigned int *gpio_base, *pwm, *clk;

void pwmSetClock (int divisor)
{
	uint32_t pwm_control ;
	divisor &= 4095 ;

  	pwm_control = *(pwm + PWM_CONTROL) ;		// preserve PWM_CONTROL
  	*(pwm + PWM_CONTROL) = 0 ;				// Stop PWM
    	*(clk + PWMCLK_CNTL) = BCM_PASSWORD | 0x01 ;	// Stop PWM Clock	
    	msleep(110) ;			// prevents clock going sloooow

	// Wait for clock to be !BUSY
    	while ((*(clk + PWMCLK_CNTL) & 0x80) != 0) msleep(1) ; msleep(10);
    	*(clk + PWMCLK_DIV)  = BCM_PASSWORD | (divisor << 12) ;

    	*(clk + PWMCLK_CNTL) = BCM_PASSWORD | 0x11 ;	// Start PWM clock
    	*(pwm + PWM_CONTROL) = pwm_control ;		// restore PWM_CONTROL 
}


int servo_open(struct inode *inode, struct file *filep) {

	int size = 4 * 1024;
	printk(KERN_ALERT "Buzzer Driver Opened\n");

	gpio_base = (uint32_t *)ioremap(GPIO_BASE_ADDR, size);
	pwm = (uint32_t *)ioremap(PWM_BASE_ADDR, size);
	clk = (uint32_t *)ioremap(CLK_BASE_ADDR, size);

	// Set alternate function of GPIO BUZZER as ALT 5
	int fsel = gpioToGPFSEL[GPIO_BUZZER];
	int shift = gpioToShift[GPIO_BUZZER];

	*(gpio_base + fsel) = (*(gpio_base + fsel) & ~(7 << shift)) | ((FSEL_ALT0 & 0x07) << shift);
	msleep(150);

	// Set pin as pwm mode (Using PWEN0 = enable, MODE0=0, MSEN0=Mark Space Mode)
	*(pwm + PWM_CONTROL) = PWM0_ENABLE | PWM0_MS_MODE | PWM1_ENABLE | PWM1_MS_MODE;	
	printk(KERN_ALERT "Default Clock Divisor : %d\n", *(clk + PWMCLK_DIV) >> 12);
	pwmSetClock(192);
	printk(KERN_ALERT "Default Clock Divisor : %d\n", *(clk + PWMCLK_DIV) >> 12);
	*(pwm + PWM1_RANGE) = 2000; msleep(10);	
	return 0;
}


int servo_release(struct inode *inode, struct file *filep) {
	printk(KERN_ALERT, "Servo Driver Released\n");
	iounmap((void *)gpio_base);
	return 0;
}

long servo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

	int note;	
	switch(cmd) {

		case BUZZER_IOCTL_CMD_PLAY:
			copy_from_user(&note, (const void*)arg, 4);
			//printk(KERN_ALERT "[%s] Playing Note :%d\n", BUZZER_DEV_NAME, note);

			*(pwm + gpioToPwmPort[GPIO_BUZZER]) = note;

			break;

		case BUZZER_IOCTL_CMD_STOP:
			
			printk(KERN_ALERT "[%s] Stop Playing", BUZZER_DEV_NAME);

			*(pwm + gpioToPwmPort[GPIO_BUZZER]) = 0;

			break;
	}

	return 0;
}
static struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.open = servo_open,
	.release = servo_release,
	.unlocked_ioctl = servo_ioctl	
};

int __init servo_init(void) {
	if(register_chrdev(BUZZER_MAJOR_NUMBER, BUZZER_DEV_NAME, &dev_fops) < 0)
		printk(KERN_ALERT "[%s] init failed\n", BUZZER_DEV_NAME);
	else
		printk(KERN_ALERT "[%s] init successful\n", BUZZER_DEV_NAME);


	return 0;
}

void __exit servo_exit(void) {
	unregister_chrdev(BUZZER_MAJOR_NUMBER, BUZZER_DEV_NAME);
	printk(KERN_ALERT, "[servo] cleanup\n");
}

module_init(servo_init);
module_exit(servo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gyu Jin Choi");
MODULE_DESCRIPTION("[WEEK8] This is the hello world example for device driver in system programmer lecture.");
