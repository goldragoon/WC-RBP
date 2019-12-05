#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/unistd.h>

#include <linux/delay.h>

#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define INFRARED_MAJOR_NUMBER 506
#define INFRARED_DEV_NAME "infrared_ioctl" 

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0x04  // INFRARED

#define GPLEV0 0x34 

#define GPSET0 0x1C
#define GPCLR0 0x28

#define IOCTL_MAGIC_NUMBER 'z'
#define IOCTL_CMD_SET_DIRECTION  _IOWR(IOCTL_MAGIC_NUMBER,0,int)
#define IOCTL_CMD_BLINK  _IOWR(IOCTL_MAGIC_NUMBER,1,int)

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpset0;
volatile unsigned int *gpclr0;
volatile unsigned int *gplev0;
int infrared_open(struct inode *inode , struct file *filp){

  printk(KERN_ALERT "INFRARED driver open!!\n");
  gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
  gpsel1 = (volatile unsigned int *)(gpio_base + GPFSEL1);
  gpset0 = (volatile unsigned int *)(gpio_base + GPSET0);
  gpclr0 = (volatile unsigned int *)(gpio_base + GPCLR0);
  gplev0 = (volatile unsigned int *)(gpio_base + GPLEV0);
  return 0;
}

int infrared_release(struct inode *inode , struct file *filp){
  printk(KERN_ALERT "INFRARED driver closed!!\n");
  iounmap((void*)gpio_base);
  return 0;
}

long infrared_ioctl(struct file *flip, unsigned int cmd , unsigned long arg){
  
  int kbuf = -1;
  int state = 1;
  switch(cmd){
    case IOCTL_CMD_SET_DIRECTION:
	    copy_from_user(&kbuf, (const void*)arg, 4);
	    if(kbuf == 0 ){
              printk(KERN_ALERT "INFRARED set direction in!!\n");
	      *gpsel1 |= (0 << 21);
	    } else if (kbuf == 1){
              printk(KERN_ALERT "INFRARED set direction out!!\n");
	      *gpsel1 |= (1 << 21);
	    } else {
              printk(KERN_ALERT "ERROR direction parameter error\n");
	      return -1;
	    }
	    break;

    case IOCTL_CMD_BLINK:
	   if(*gplev0 & ( 1 << 17)){
                state = 1;
                copy_to_user((const void*)arg, &state, 4);
		printk(KERN_ALERT "STATE ON\n");
           } else {
                state = 0;
                copy_to_user((const void*)arg, &state, 4);
		printk(KERN_ALERT "STATE OFF\n");
	   }
	   break;
  }
  return 0;
}

static struct file_operations infrared_fops = {
  .owner = THIS_MODULE,
  .open = infrared_open,
  .release = infrared_release,
  .unlocked_ioctl = infrared_ioctl,

};

int __init infrared_init(void){

  if(register_chrdev(INFRARED_MAJOR_NUMBER , INFRARED_DEV_NAME , &infrared_fops)<0){
   printk(KERN_ALERT "INFRARED driver initialization fail\n");
  } else {
   printk(KERN_ALERT "INFRARED driver initialization success\n");

  }

  return 0;
}
void __exit infrared_exit(void){
  unregister_chrdev(INFRARED_MAJOR_NUMBER, INFRARED_DEV_NAME);
   printk(KERN_ALERT "INFRARED driver exit done\n");

}

module_init(infrared_init);
module_exit(infrared_exit);
