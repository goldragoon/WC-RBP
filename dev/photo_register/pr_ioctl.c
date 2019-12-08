#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/unistd.h>

#include <linux/delay.h>
#include "bcm2835.h"
#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define PR_MAJOR_NUMBER 503
#define PR_DEV_NAME "pr_ioctl" 

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL2 0x08  // PR

#define GPLEV0 0x34 

#define GPSET0 0x1C
#define GPCLR0 0x28

#define IOCTL_MAGIC_NUMBER 'k'
#define IOCTL_CMD_SET_DIRECTION  _IOWR(IOCTL_MAGIC_NUMBER,0,int)
#define IOCTL_CMD_INITIATE  _IOWR(IOCTL_MAGIC_NUMBER,1,int)
#define IOCTL_CMD_CHECK_LIGHT  _IOWR(IOCTL_MAGIC_NUMBER,2,int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;
int pr_open(struct inode *inode , struct file *filp){

  printk(KERN_ALERT "PR driver open!!\n");
  gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
  gpsel2 = (volatile unsigned int *)(gpio_base + GPFSEL2);
  gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
  gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
  gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
  return 0;
}

int pr_release(struct inode *inode , struct file *filp){
  printk(KERN_ALERT "PR driver closed!!\n");
  iounmap((void*)gpio_base);
  return 0;
}

long pr_ioctl(struct file *flip, unsigned int cmd , unsigned long arg){

  int kbuf = -1;
  int state = 0;
  switch(cmd){
    case IOCTL_CMD_SET_DIRECTION:
	    copy_from_user(&kbuf, (const void*)arg, 4);
	    if(kbuf == 0 ){
              printk(KERN_ALERT "PR set direction in!!\n");
	      pinModeAlt(gpio_base, 22, FSEL_INPT);
	    } else if (kbuf == 1){
              printk(KERN_ALERT "PR set direction out!!\n");
	pinModeAlt(gpio_base, 22, FSEL_OUTP);
	      
	    } else {
              printk(KERN_ALERT "ERROR direction parameter error\n");            return -1;
	    }
	    break;
    case IOCTL_CMD_INITIATE:
	    copy_from_user(&kbuf, (const void*)arg, 4);
	    if(kbuf == 0 ){
              printk(KERN_ALERT "PR initiate 0\n");
	      *gpclr0 |=  ( 1 << 22 );
	    } else if (kbuf == 1){
              printk(KERN_ALERT "PR initiate 1\n");
	      *gpset0 |= (1 << 22);
	    } else {
              printk(KERN_ALERT "ERROR direction parameter error\n");            return -1;
	    }
	    break;
    case IOCTL_CMD_CHECK_LIGHT:
	    if(*gplev0 & (1 << 22)){
                state = 1;
                copy_to_user((const void*)arg, &state, 4);
		printk(KERN_ALERT "High\n");
            } else {
                state = 0;
                copy_to_user((const void*)arg, &state, 4);
		printk(KERN_ALERT "Low\n");
	    }
	    break; 	    
  }
  return 0;
}

static struct file_operations pr_fops = {
  .owner = THIS_MODULE,
  .open = pr_open,
  .release = pr_release,
  .unlocked_ioctl = pr_ioctl,
};

int __init pr_init(void){

  if(register_chrdev(PR_MAJOR_NUMBER , PR_DEV_NAME , &pr_fops)<0){
   printk(KERN_ALERT "PR driver initialization fail\n");
  } else {
   printk(KERN_ALERT "PR driver initialization success\n");

  }
  return 0;
}
void __exit pr_exit(void){
  unregister_chrdev(PR_MAJOR_NUMBER, PR_DEV_NAME);
   printk(KERN_ALERT "PR driver exit done\n");

}

module_init(pr_init);
module_exit(pr_exit);
