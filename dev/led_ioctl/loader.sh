sudo rmmod led_ioctl
make clean
make
sudo insmod led_ioctl.ko
