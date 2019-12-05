sudo rmmod sonar_ioctl
make clean
make
sudo insmod sonar_ioctl.ko
