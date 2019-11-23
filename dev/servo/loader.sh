sudo rmmod servo_ioctl
make clean
make
sudo insmod servo_ioctl.ko
