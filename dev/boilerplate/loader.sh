sudo rmmod touch_ioctl
make clean
make
sudo insmod touch_ioctl.ko
