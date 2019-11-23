#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

// Servo Related
#define SERVO_MAJOR_NUMBER 		500
#define SERVO_MINOR_NUMBER 		100
#define SERVO_DEV_PATH_NAME 		"/dev/servo_ioctl"
#define SERVO_IOCTL_MAGIC_NUMBER 	's'
#define SERVO_IOCTL_CMD_ROTATE 		_IOWR(SERVO_IOCTL_MAGIC_NUMBER, 0, int)

// Touch Related
#define TOUCH_MAJOR_NUMBER 		501
#define TOUCH_MINOR_NUMBER		100
#define TOUCH_DEV_PATH_NAME 		"/dev/touch_ioctl"
#define TOUCH_IOCTL_MAGIC_NUMBER 	't'
#define TOUCH_IOCTL_IS_TOUCHED 		_IOWR(TOUCH_IOCTL_MAGIC_NUMBER, 0, int)


// LED Related
#define LED_MAJOR_NUMBER 		504
#define LED_MINOR_NUMBER		100
#define LED_DEV_PATH_NAME 		"/dev/led_ioctl"
#define LED_IOCTL_MAGIC_NUMBER 	'm'
#define LED_IOCTL_IS_TOUCHED 		_IOWR(TOUCH_IOCTL_MAGIC_NUMBER, 0, int)

int open_dev(int major_number, int minor_number, char *dev_path_name) {
	dev_t dev;
	int fd;
	dev = makedev(major_number, minor_number);
	mknod(dev_path_name, S_IFCHR | 0666, dev);	
	fd = open(dev_path_name, O_RDWR);
	return fd;
	

}

int main(int argc, char **argv) {
	
	// Initialize Servo

	int servo_fd;
	servo_fd = open_dev(SERVO_MAJOR_NUMBER, SERVO_MINOR_NUMBER, SERVO_DEV_PATH_NAME);
	if(servo_fd < 0) { printf("Failed to open %s.\n", SERVO_DEV_PATH_NAME); return -1; }

	// Initialize Touch
	
	int touch_fd;
	touch_fd = open_dev(TOUCH_MAJOR_NUMBER, TOUCH_MINOR_NUMBER, TOUCH_DEV_PATH_NAME);	
	if(touch_fd < 0) { printf("Failed to open %s.\n", TOUCH_DEV_PATH_NAME); return -1; }

	// Initialize LED
	
	int led_fd;
	led_fd = open_dev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER, LED_DEV_PATH_NAME);	
	if(led_fd < 0) { printf("Failed to open %s.\n", LED_DEV_PATH_NAME); return -1; }
	

	while(true) {

		int isTouched = 0;
		for(int angle = 80; angle < 220; angle+=5) {
			printf("Angle : %d\n", angle);
			ioctl(servo_fd, SERVO_IOCTL_CMD_ROTATE, &angle);
			ioctl(touch_fd, TOUCH_IOCTL_IS_TOUCHED, &isTouched);
			printf("isTouched : %d\n", isTouched);

			sleep(1);
		}

	}
	
	close(servo_fd);
	return 0;
	
}
