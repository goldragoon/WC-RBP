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

// Buzzer Related
#define BUZZER_MAJOR_NUMBER 		502
#define BUZZER_MINOR_NUMBER 		100
#define BUZZER_DEV_PATH_NAME 		"/dev/buzzer_ioctl"
#define BUZZER_IOCTL_MAGIC_NUMBER 	'p'
#define BUZZER_IOCTL_CMD_PLAY 		_IOWR(BUZZER_IOCTL_MAGIC_NUMBER, 0, int)
#define BUZZER_IOCTL_CMD_STOP		_IOWR(BUZZER_IOCTL_MAGIC_NUMBER, 1, int)

// Photo Register Related 
#define PR_MAJOR_NUMBER 		503
#define PR_MINOR_NUMBER		        100
#define PR_DEV_PATH_NAME 		"/dev/pr_ioctl"
#define PR_IOCTL_MAGIC_NUMBER 		'k'
#define PR_IOCTL_CMD_SET_DIRECTION   _IOWR(PR_IOCTL_MAGIC_NUMBER, 0, int)
#define PR_IOCTL_CMD_INITIATE           _IOWR(PR_IOCTL_MAGIC_NUMBER, 1,int)
#define PR_IOCTL_CMD_CHECK_LIGHT           _IOWR(PR_IOCTL_MAGIC_NUMBER, 2,int)


// LED Related
#define LED_MAJOR_NUMBER 		504
#define LED_MINOR_NUMBER		100
#define LED_DEV_PATH_NAME 		"/dev/led_ioctl"
#define LED_IOCTL_MAGIC_NUMBER 		'm'
#define LED_IOCTL_CMD_SET_DIRECTION   _IOWR(LED_IOCTL_MAGIC_NUMBER, 0, int)
#define LED_IOCTL_CMD_BLINK           _IOWR(LED_IOCTL_MAGIC_NUMBER, 1,int)


// INFRARED Sensor Related
#define INFRARED_MAJOR_NUMBER 		506
#define INFRARED_MINOR_NUMBER		100
#define INFRARED_DEV_PATH_NAME 		"/dev/infrared_ioctl"
#define INFRARED_IOCTL_MAGIC_NUMBER 		'z'
#define INFRARED_IOCTL_CMD_SET_DIRECTION   _IOWR(INFRARED_IOCTL_MAGIC_NUMBER, 0, int)
#define INFRARED_IOCTL_CMD_BLINK           _IOWR(INFRARED_IOCTL_MAGIC_NUMBER, 1,int)

int rc_time(int pr_fd){

   int count = 0;
   int pin_direction;
   int init;
   //Output on the pin for 
   pin_direction = 1;
   ioctl(pr_fd, PR_IOCTL_CMD_SET_DIRECTION, &pin_direction);
   init = 0;
   ioctl(pr_fd, PR_IOCTL_CMD_INITIATE,&init);
   usleep(50000);

   //Change the pin back to input
   pin_direction = 0;
   ioctl(pr_fd, PR_IOCTL_CMD_SET_DIRECTION, &pin_direction);

   //Count until the pin goes high
   int state = 0;
   //clock_t start, end;
   //start = clock();
   while(true){
     ioctl(pr_fd, PR_IOCTL_CMD_CHECK_LIGHT, &state);
     if(state != 0){
       break;
     }
     count += 1;
   }
   //end = clock();
   //float ref = (float)(end - start)/CLOCKS_PER_SEC;
   //printf("%.3f\n", ref);
   return count;
}

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

	// Initialize Buzzer
	
	int buzzer_fd;
	buzzer_fd = open_dev(BUZZER_MAJOR_NUMBER, BUZZER_MINOR_NUMBER, BUZZER_DEV_PATH_NAME);	
	if(buzzer_fd < 0) { printf("Failed to open %s.\n", BUZZER_DEV_PATH_NAME); return -1; }
	
	// Initialize LED	
	int led_fd;
	led_fd = open_dev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER, LED_DEV_PATH_NAME);	
	if(led_fd < 0) { printf("Failed to open %s.\n", LED_DEV_PATH_NAME); return -1; }

	// Initialize INFRARED	
        int infrared_fd;
        infrared_fd = open_dev(INFRARED_MAJOR_NUMBER, INFRARED_MINOR_NUMBER, INFRARED_DEV_PATH_NAME);	
        if(infrared_fd < 0) { printf("Failed to open %s.\n", INFRARED_DEV_PATH_NAME); return -1; }

	// Initialize PR(Photo Register)
        int pr_fd;
        pr_fd = open_dev(PR_MAJOR_NUMBER, PR_MINOR_NUMBER, PR_DEV_PATH_NAME);	
        if(pr_fd < 0) { printf("Failed to open %s.\n", PR_DEV_PATH_NAME); return -1; }
	while(true) {

		int isTouched = 0;
		for(int angle = 80; angle < 220; angle+=5) {
			printf("Angle : %d\n", angle);
			ioctl(servo_fd, SERVO_IOCTL_CMD_ROTATE, &angle);
			ioctl(touch_fd, TOUCH_IOCTL_IS_TOUCHED, &isTouched);
			ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &angle);
			printf("isTouched : %d\n", isTouched);

			sleep(1);
		}

	}
	
	close(servo_fd);
	return 0;
	
}
