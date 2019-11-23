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

#define LED_MAJOR_NUMBER 503
#define LED_DEV_NAME "pr_ioctl" 

#define IOCTL_MAGIC_NUMBER 'k'
#define IOCTL_CMD_SET_DIRECTION  _IOWR(IOCTL_MAGIC_NUMBER,0,int)

