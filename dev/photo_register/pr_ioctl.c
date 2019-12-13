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
#define IOCTL_CMD_CHECK_LIGHT  _IOWR(IOCTL_MAGIC_NUMBER,0,int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel2;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;
int rc_time(void);

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
  int count = 0;
  switch(cmd){

    case IOCTL_CMD_CHECK_LIGHT:
	    count = rc_time();
	    copy_to_user((const void*)arg, &count, 4);
	    printk(KERN_ALERT "CHECK_LIGHT\n"); 
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

int rc_time(){

   //Output on the pin for
   printk(KERN_ALERT "PR set direction out!!\n");
   pinModeAlt(gpio_base, 22, FSEL_OUTP);
   printk(KERN_ALERT "PR initiate 0\n");
   *gpclr0 |=  ( 1 << 22 );
   ssleep(1);

   //Change the pin back to input
   printk(KERN_ALERT "PR set direction in!!\n");
   pinModeAlt(gpio_base, 22, FSEL_INPT);

   //Count until the pin goes high
   int count = 0;
   while(true){
     if(count > 100000000){
       return -1;
     }
     if(*gplev0 & (1 << 22)){
       //printk(KERN_ALERT "High\n");
       break;
     }
     count += 1;
     //printk(KERN_ALERT "Low\n");  
   }
   return count;
}

module_init(pr_init);
module_exit(pr_exit);
