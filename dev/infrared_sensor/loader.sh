sudo rmmod infrared_ioctl
make clean
make
sudo insmod infrared_ioctl.ko
