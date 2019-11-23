sudo rmmod buzzer_ioctl
make clean
make
sudo insmod buzzer_ioctl.ko
