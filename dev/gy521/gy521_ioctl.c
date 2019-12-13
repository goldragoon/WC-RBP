#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/compiler.h>
#define I2C_RETRIES	0x0701
#define I2C_TIMEOUT	0x0702
#define I2C_SLAVE	0x0703
#define I2C_SLAVE_FORCE	0x0706
#define I2C_TENBIT	0x0704	
#define I2C_FUNCS	0x0705
#define I2C_RDWR	0x0707
#define I2C_PEC		0x0708
#define I2C_SMBUS	0x0720
#define I2C_MAJOR	499

struct i2c_smbus_ioctl_data {
	__u8 read_write;
	__u8 command;
	__u32 size;
	union i2c_smbus_data __user *data;
};


struct i2c_rdwr_ioctl_data {
	struct i2c_msg __user *msgs;
	__u32 nmsgs;		
};

#define  I2C_RDWR_IOCTL_MAX_MSGS	42
#define  I2C_RDRW_IOCTL_MAX_MSGS	I2C_RDWR_IOCTL_MAX_MSGS

struct i2c_dev {
	struct list_head list;
	struct i2c_adapter *adap;
	struct device *dev;
	struct cdev cdev;
};

#define I2C_MINORS	MINORMASK
static LIST_HEAD(i2c_dev_list);
static DEFINE_SPINLOCK(i2c_dev_list_lock);

static struct i2c_dev *i2c_dev_get_by_minor(unsigned index)
{
	struct i2c_dev *i2c_dev;

	spin_lock(&i2c_dev_list_lock);
	list_for_each_entry(i2c_dev, &i2c_dev_list, list) {
		if (i2c_dev->adap->nr == index)
			goto found;
	}
	i2c_dev = NULL;
found:
	spin_unlock(&i2c_dev_list_lock);
	return i2c_dev;
}

static struct i2c_dev *get_free_i2c_dev(struct i2c_adapter *adap)
{
	struct i2c_dev *i2c_dev;

	if (adap->nr >= I2C_MINORS) {
		printk(KERN_ERR "i2c-dev: Out of device minors (%d)\n",
		       adap->nr);
		return ERR_PTR(-ENODEV);
	}

	i2c_dev = kzalloc(sizeof(*i2c_dev), GFP_KERNEL);
	if (!i2c_dev)
		return ERR_PTR(-ENOMEM);
	i2c_dev->adap = adap;

	spin_lock(&i2c_dev_list_lock);
	list_add_tail(&i2c_dev->list, &i2c_dev_list);
	spin_unlock(&i2c_dev_list_lock);
	return i2c_dev;
}

static void put_i2c_dev(struct i2c_dev *i2c_dev)
{
	spin_lock(&i2c_dev_list_lock);
	list_del(&i2c_dev->list);
	spin_unlock(&i2c_dev_list_lock);
	kfree(i2c_dev);
}

static ssize_t name_show(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct i2c_dev *i2c_dev = i2c_dev_get_by_minor(MINOR(dev->devt));

	if (!i2c_dev)
		return -ENODEV;
	return sprintf(buf, "%s\n", i2c_dev->adap->name);
}
static DEVICE_ATTR_RO(name);

static struct attribute *i2c_attrs[] = {
	&dev_attr_name.attr,
	NULL,
};
ATTRIBUTE_GROUPS(i2c);

static int i2cdev_check(struct device *dev, void *addrp)
{
	struct i2c_client *client = i2c_verify_client(dev);

	if (!client || client->addr != *(unsigned int *)addrp)
		return 0;

	return dev->driver ? -EBUSY : 0;
}

/* walk up mux tree */
static int i2cdev_check_mux_parents(struct i2c_adapter *adapter, int addr)
{
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);
	int result;

	result = device_for_each_child(&adapter->dev, &addr, i2cdev_check);
	if (!result && parent)
		result = i2cdev_check_mux_parents(parent, addr);

	return result;
}

/* recurse down mux tree */
static int i2cdev_check_mux_children(struct device *dev, void *addrp)
{
	int result;

	if (dev->type == &i2c_adapter_type)
		result = device_for_each_child(dev, addrp,
						i2cdev_check_mux_children);
	else
		result = i2cdev_check(dev, addrp);

	return result;
}

static int i2cdev_check_addr(struct i2c_adapter *adapter, unsigned int addr)
{
	struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);
	int result = 0;

	if (parent)
		result = i2cdev_check_mux_parents(parent, addr);

	if (!result)
		result = device_for_each_child(&adapter->dev, &addr,
						i2cdev_check_mux_children);

	return result;
}

static noinline int i2cdev_ioctl_smbus(struct i2c_client *client,
		u8 read_write, u8 command, u32 size,
		union i2c_smbus_data __user *data)
{
	union i2c_smbus_data temp = {};
	int datasize, res;

	if ((size != I2C_SMBUS_BYTE) &&
	    (size != I2C_SMBUS_QUICK) &&
	    (size != I2C_SMBUS_BYTE_DATA) &&
	    (size != I2C_SMBUS_WORD_DATA) &&
	    (size != I2C_SMBUS_PROC_CALL) &&
	    (size != I2C_SMBUS_BLOCK_DATA) &&
	    (size != I2C_SMBUS_I2C_BLOCK_BROKEN) &&
	    (size != I2C_SMBUS_I2C_BLOCK_DATA) &&
	    (size != I2C_SMBUS_BLOCK_PROC_CALL)) {
		dev_dbg(&client->adapter->dev,
			"size out of range (%x) in ioctl I2C_SMBUS.\n",
			size);
		return -EINVAL;
	}

	if ((read_write != I2C_SMBUS_READ) &&
	    (read_write != I2C_SMBUS_WRITE)) {
		dev_dbg(&client->adapter->dev,
			"read_write out of range (%x) in ioctl I2C_SMBUS.\n",
			read_write);
		return -EINVAL;
	}

	if ((size == I2C_SMBUS_QUICK) ||
	    ((size == I2C_SMBUS_BYTE) &&
	    (read_write == I2C_SMBUS_WRITE)))
		/* These are special: we do not use data */
		return i2c_smbus_xfer(client->adapter, client->addr,
				      client->flags, read_write,
				      command, size, NULL);

	if (data == NULL) {
		dev_dbg(&client->adapter->dev,
			"data is NULL pointer in ioctl I2C_SMBUS.\n");
		return -EINVAL;
	}

	if ((size == I2C_SMBUS_BYTE_DATA) ||
	    (size == I2C_SMBUS_BYTE))
		datasize = sizeof(data->byte);
	else if ((size == I2C_SMBUS_WORD_DATA) ||
		 (size == I2C_SMBUS_PROC_CALL))
		datasize = sizeof(data->word);
	else /* size == smbus block, i2c block, or block proc. call */
		datasize = sizeof(data->block);

	if ((size == I2C_SMBUS_PROC_CALL) ||
	    (size == I2C_SMBUS_BLOCK_PROC_CALL) ||
	    (size == I2C_SMBUS_I2C_BLOCK_DATA) ||
	    (read_write == I2C_SMBUS_WRITE)) {
		if (copy_from_user(&temp, data, datasize))
			return -EFAULT;
	}
	if (size == I2C_SMBUS_I2C_BLOCK_BROKEN) {
		/* Convert old I2C block commands to the new
		   convention. This preserves binary compatibility. */
		size = I2C_SMBUS_I2C_BLOCK_DATA;
		if (read_write == I2C_SMBUS_READ)
			temp.block[0] = I2C_SMBUS_BLOCK_MAX;
	}
	res = i2c_smbus_xfer(client->adapter, client->addr, client->flags,
	      read_write, command, size, &temp);
	if (!res && ((size == I2C_SMBUS_PROC_CALL) ||
		     (size == I2C_SMBUS_BLOCK_PROC_CALL) ||
		     (read_write == I2C_SMBUS_READ))) {
		if (copy_to_user(data, &temp, datasize))
			return -EFAULT;
	}
	return res;
}

static long i2cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = file->private_data;

	dev_dbg(&client->adapter->dev, "ioctl, cmd=0x%02x, arg=0x%02lx\n",
		cmd, arg);

	switch (cmd) {
	case I2C_SLAVE:
	case I2C_SLAVE_FORCE:
		if ((arg > 0x3ff) ||
		    (((client->flags & I2C_M_TEN) == 0) && arg > 0x7f))
			return -EINVAL;
		if (cmd == I2C_SLAVE && i2cdev_check_addr(client->adapter, arg))
			return -EBUSY;
		/* REVISIT: address could become busy later */
		client->addr = arg;
		return 0;
	case I2C_SMBUS: {
		struct i2c_smbus_ioctl_data data_arg;
		if (copy_from_user(&data_arg,
				   (struct i2c_smbus_ioctl_data __user *) arg,
				   sizeof(struct i2c_smbus_ioctl_data)))
			return -EFAULT;
		return i2cdev_ioctl_smbus(client, data_arg.read_write,
					  data_arg.command,
					  data_arg.size,
					  data_arg.data);
	}
	default:
		return -ENOTTY;
	}
	return 0;
}

static int i2cdev_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct i2c_client *client;
	struct i2c_adapter *adap;

	adap = i2c_get_adapter(minor);
	if (!adap)
		return -ENODEV;
	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client) {
		i2c_put_adapter(adap);
		return -ENOMEM;
	}
	snprintf(client->name, I2C_NAME_SIZE, "i2c-dev %d", adap->nr);

	client->adapter = adap;
	file->private_data = client;

	return 0;
}

static int i2cdev_release(struct inode *inode, struct file *file)
{
	struct i2c_client *client = file->private_data;

	i2c_put_adapter(client->adapter);
	kfree(client);
	file->private_data = NULL;

	return 0;
}

static const struct file_operations i2cdev_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.unlocked_ioctl	= i2cdev_ioctl,
	.open		= i2cdev_open,
	.release	= i2cdev_release,
};

/* ------------------------------------------------------------------------- */

static struct class *i2c_dev_class;

static int i2cdev_attach_adapter(struct device *dev, void *dummy)
{
	struct i2c_adapter *adap;
	struct i2c_dev *i2c_dev;
	int res;

	if (dev->type != &i2c_adapter_type)
		return 0;
	adap = to_i2c_adapter(dev);

	i2c_dev = get_free_i2c_dev(adap);
	if (IS_ERR(i2c_dev))
		return PTR_ERR(i2c_dev);

	cdev_init(&i2c_dev->cdev, &i2cdev_fops);
	i2c_dev->cdev.owner = THIS_MODULE;
	res = cdev_add(&i2c_dev->cdev, MKDEV(I2C_MAJOR, adap->nr), 1);
	if (res)
		goto error_cdev;

	/* register this i2c device with the driver core */
	i2c_dev->dev = device_create(i2c_dev_class, &adap->dev,
				     MKDEV(I2C_MAJOR, adap->nr), NULL,
				     "i2c-%d", adap->nr + 3);
	if (IS_ERR(i2c_dev->dev)) {
		res = PTR_ERR(i2c_dev->dev);
		goto error;
	}

	pr_debug("i2c-dev: adapter [%s] registered as minor %d\n",
		 adap->name, adap->nr);
	return 0;
error:
	cdev_del(&i2c_dev->cdev);
error_cdev:
	put_i2c_dev(i2c_dev);
	return res;
}

static int i2cdev_detach_adapter(struct device *dev, void *dummy)
{
	struct i2c_adapter *adap;
	struct i2c_dev *i2c_dev;

	if (dev->type != &i2c_adapter_type)
		return 0;
	adap = to_i2c_adapter(dev);

	i2c_dev = i2c_dev_get_by_minor(adap->nr);
	if (!i2c_dev) /* attach_adapter must have failed */
		return 0;

	cdev_del(&i2c_dev->cdev);
	put_i2c_dev(i2c_dev);
	device_destroy(i2c_dev_class, MKDEV(I2C_MAJOR, adap->nr));

	pr_debug("i2c-dev: adapter [%s] unregistered\n", adap->name);
	return 0;
}

static int i2cdev_notifier_call(struct notifier_block *nb, unsigned long action,
			 void *data)
{
	struct device *dev = data;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
		return i2cdev_attach_adapter(dev, NULL);
	case BUS_NOTIFY_DEL_DEVICE:
		return i2cdev_detach_adapter(dev, NULL);
	}

	return 0;
}

static struct notifier_block i2cdev_notifier = {
	.notifier_call = i2cdev_notifier_call,
};

static int __init i2c_dev_init(void)
{
	int res;

	printk(KERN_INFO "i2c /dev entries driver\n");
	res = register_chrdev_region(MKDEV(I2C_MAJOR, 0), I2C_MINORS, "i2c");
	if (res)
		goto out;

	i2c_dev_class = class_create(THIS_MODULE, "i2c-dev");
	if (IS_ERR(i2c_dev_class)) {
		res = PTR_ERR(i2c_dev_class);
		goto out_unreg_chrdev;
	}
	i2c_dev_class->dev_groups = i2c_groups;

	/* Keep track of adapters which will be added or removed later */
	res = bus_register_notifier(&i2c_bus_type, &i2cdev_notifier);
	if (res)
		goto out_unreg_class;

	/* Bind to already existing adapters right away */
	i2c_for_each_dev(NULL, i2cdev_attach_adapter);

	return 0;

out_unreg_class:
	class_destroy(i2c_dev_class);
out_unreg_chrdev:
	unregister_chrdev_region(MKDEV(I2C_MAJOR, 0), I2C_MINORS);
out:
	printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);
	return res;
}

static void __exit i2c_dev_exit(void)
{
	bus_unregister_notifier(&i2c_bus_type, &i2cdev_notifier);
	i2c_for_each_dev(NULL, i2cdev_detach_adapter);
	class_destroy(i2c_dev_class);
	unregister_chrdev_region(MKDEV(I2C_MAJOR, 0), I2C_MINORS);
}

MODULE_AUTHOR("Gyu Jin Choi");
MODULE_DESCRIPTION("I2C driver");
MODULE_LICENSE("GPL");
module_init(i2c_dev_init);
module_exit(i2c_dev_exit);
