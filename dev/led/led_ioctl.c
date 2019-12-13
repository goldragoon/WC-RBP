#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/unistd.h>

#include <linux/delay.h>
#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define LED_MAJOR_NUMBER 504
#define LED_DEV_NAME "led_ioctl" 

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL0 0x00  // LED 

#define GPLEV0 0x34 

#define GPSET0 0x1C
#define GPCLR0 0x28

#define IOCTL_MAGIC_NUMBER 'm'
#define IOCTL_CMD_SET_DIRECTION  _IOWR(IOCTL_MAGIC_NUMBER,0,int)
#define IOCTL_CMD_BLINK  _IOWR(IOCTL_MAGIC_NUMBER,1,int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel0;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;
volatile unsigned int *gplev0;
int led_open(struct inode *inode , struct file *filp){

  printk(KERN_ALERT "LED driver open!!\n");
  gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
  gpsel0 = (volatile unsigned int *)(gpio_base + GPFSEL0);
  gpset1 = (volatile unsigned int *)(gpio_base + GPSET0);
  gpclr1 = (volatile unsigned int *)(gpio_base + GPCLR0);
  gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
  *gpsel0 |= (1 << 15);
  return 0;
}

int led_release(struct inode *inode , struct file *filp){
  printk(KERN_ALERT "LED driver closed!!\n");
  iounmap((void*)gpio_base);
  return 0;
}

long led_ioctl(struct file *flip, unsigned int cmd , unsigned long arg){
  
  int kbuf = -1;
	printk(KERN_ALERT, "cmd : %d", cmd);
  switch(cmd){
    case IOCTL_CMD_SET_DIRECTION:
	    copy_from_user(&kbuf, (const void*)arg, 4);
	    if(kbuf == 0 ){
              printk(KERN_ALERT "LED set direction in!!\n");
	      *gpsel0 |= (0 << 15);
	    } else if (kbuf == 1){
              printk(KERN_ALERT "LED set direction out!!\n");
	      *gpsel0 |= (1 << 15);
	    } else {
              printk(KERN_ALERT "ERROR direction parameter error\n");
	      return -1;
	    }
	    break;

    case IOCTL_CMD_BLINK:
	   copy_from_user(&kbuf, (const void*)arg, 4);
           if( kbuf == 0 ){
             printk(KERN_ALERT "LED OFF\n");
	     *gpclr1 |= ( 1 << 5 );
	   } else {
             printk(KERN_ALERT "LED ON\n");
	     *gpset1 |= ( 1 << 5 );
	   }
	   break;
  }
  return 0;
}

static struct file_operations led_fops = {
  .owner = THIS_MODULE,
  .open = led_open,
  .release = led_release,
  .unlocked_ioctl = led_ioctl,

};

int __init led_init(void){

  if(register_chrdev(LED_MAJOR_NUMBER , LED_DEV_NAME , &led_fops)<0){
   printk(KERN_ALERT "LED driver initialization fail\n");
  } else {
   printk(KERN_ALERT "LED driver initialization success\n");

  }

  return 0;
}
void __exit led_exit(void){
  unregister_chrdev(LED_MAJOR_NUMBER, LED_DEV_NAME);
   printk(KERN_ALERT "LED driver exit done\n");

}

module_init(led_init);
module_exit(led_exit);

