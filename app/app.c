#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>

#define SENSOR_COUNT 8
#define SENSOR_SERVO_IDX 0
#define SENSOR_TOUCH_IDX 1
#define SENSOR_PR_IDX 2
#define SENSOR_LED_IDX 	 3
#define SENSOR_SONAR_IDX 4
#define SENSOR_INFRARED_IDX 5
#define SENSOR_DHT11_IDX 6
#define SENSOR_BUZZER_IDX 7

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
#define PR_IOCTL_CMD_CHECK_LIGHT           _IOWR(PR_IOCTL_MAGIC_NUMBER,0,int)


// LED Related
#define LED_MAJOR_NUMBER 		504
#define LED_MINOR_NUMBER		100
#define LED_DEV_PATH_NAME 		"/dev/led_ioctl"
#define LED_IOCTL_MAGIC_NUMBER 		'm'
#define LED_IOCTL_CMD_SET_DIRECTION   _IOWR(LED_IOCTL_MAGIC_NUMBER, 0, int)
#define LED_IOCTL_CMD_BLINK           _IOWR(LED_IOCTL_MAGIC_NUMBER, 1,int)

// Sonar Related
#define SONAR_SENSOR_COUNT		3
#define SONAR_MAJOR_NUMBER 		505
#define SONAR_MINOR_NUMBER 		100
#define SONAR_DEV_PATH_NAME		"/dev/sonar_ioctl"
#define SONAR_IOCTL_MAGIC_NUMBER 	'o'
#define SONAR_IOCTL_READ_DIST 		_IOWR(SONAR_IOCTL_MAGIC_NUMBER, 0, int)
#define SONAR_IOCTL_SET_TARGET 		_IOWR(SONAR_IOCTL_MAGIC_NUMBER, 1, int)

// INFRARED Sensor Related
#define INFRARED_MAJOR_NUMBER 		506
#define INFRARED_MINOR_NUMBER		100
#define INFRARED_DEV_PATH_NAME 		"/dev/infrared_ioctl"
#define INFRARED_IOCTL_MAGIC_NUMBER 		'z'
#define INFRARED_IOCTL_CMD_SET_DIRECTION   _IOWR(INFRARED_IOCTL_MAGIC_NUMBER, 0, int)
#define INFRARED_IOCTL_CMD_CHECK_MOVEMENT  _IOWR(INFRARED_IOCTL_MAGIC_NUMBER, 1,int)

// DHT11 Related

#define DHT11_MAJOR_NUMBER 		507
#define DHT11_MINOR_NUMBER 		100
#define	DHT11_DEV_PATH_NAME 			"/dev/dht11_ioctl"
#define DHT11_IOCTL_MAGIC_NUMBER 	'x'
#define DHT11_IOCTL_READ_TEMP 		_IOWR(DHT11_IOCTL_MAGIC_NUMBER, 0, int)


pthread_t threads[SENSOR_COUNT];
int servo_fd, touch_fd, buzzer_fd, led_fd, sonar_fd, infrared_fd, pr_fd, dht11_fd;
void* servo_start_routine(void*);
void* touch_start_routine(void*);
void* sonar_start_routine(void*);
void* infrared_start_routine(void*);
void* pr_start_routine(void*);
void* dht11_start_routine(void*);

int isTouched = 0;		// Touch sensor
int isDark = 0;			// Photo resistor
int isSomethingMoved = 0;	// Infrared
int temperature = 0;		// DHT11
int darkness = 0;		// photo resistor
unsigned int sonar_dists[SONAR_SENSOR_COUNT];

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
	servo_fd = open_dev(SERVO_MAJOR_NUMBER, SERVO_MINOR_NUMBER, SERVO_DEV_PATH_NAME);
	if(servo_fd < 0) { printf("Failed to open %s.\n", SERVO_DEV_PATH_NAME); return -1; }
	pthread_create(&threads[SENSOR_SERVO_IDX], NULL, servo_start_routine, NULL);

		
	// Initialize Touch
	touch_fd = open_dev(TOUCH_MAJOR_NUMBER, TOUCH_MINOR_NUMBER, TOUCH_DEV_PATH_NAME);	
	if(touch_fd < 0) { printf("Failed to open %s.\n", TOUCH_DEV_PATH_NAME); return -1; }	
	pthread_create(&threads[SENSOR_TOUCH_IDX], NULL, touch_start_routine, NULL);
	
        	
	// Initialize Buzzer
	
	buzzer_fd = open_dev(BUZZER_MAJOR_NUMBER, BUZZER_MINOR_NUMBER, BUZZER_DEV_PATH_NAME);	
	if(buzzer_fd < 0) { printf("Failed to open %s.\n", BUZZER_DEV_PATH_NAME); return -1; }
	
	
	// Initialize LED
	
	led_fd = open_dev(LED_MAJOR_NUMBER, LED_MINOR_NUMBER, LED_DEV_PATH_NAME);	
	if(led_fd < 0) { printf("Failed to open %s.\n", LED_DEV_PATH_NAME); return -1; }	
	

	// Initialize SONAR

	sonar_fd = open_dev(SONAR_MAJOR_NUMBER, SONAR_MINOR_NUMBER, SONAR_DEV_PATH_NAME);	
	if(sonar_fd < 0) { printf("Failed to open %s.\n", SONAR_DEV_PATH_NAME); return -1; }
	pthread_create(&threads[SENSOR_SONAR_IDX], NULL, sonar_start_routine, NULL);
        
	// Initialize INFRARED	 
        
	infrared_fd = open_dev(INFRARED_MAJOR_NUMBER, INFRARED_MINOR_NUMBER, INFRARED_DEV_PATH_NAME);	
        if(infrared_fd < 0) { printf("Failed to open %s.\n", INFRARED_DEV_PATH_NAME); return -1; }
	pthread_create(&threads[SENSOR_INFRARED_IDX], NULL, infrared_start_routine, NULL);
	
	// Initialize PR(Photo Register)
       	 
        pr_fd = open_dev(PR_MAJOR_NUMBER, PR_MINOR_NUMBER, PR_DEV_PATH_NAME);	
        if(pr_fd < 0) { printf("Failed to open %s.\n", PR_DEV_PATH_NAME); return -1; }
	pthread_create(&threads[SENSOR_PR_IDX], NULL, pr_start_routine, NULL);

	// Initialize Temparature
        
	dht11_fd = open_dev(DHT11_MAJOR_NUMBER, DHT11_MINOR_NUMBER, DHT11_DEV_PATH_NAME);	
        if(dht11_fd < 0) { printf("Failed to open %s.\n", DHT11_DEV_PATH_NAME); return -1; }
	pthread_create(&threads[SENSOR_DHT11_IDX], NULL, dht11_start_routine, NULL);
	
	for(int i = 0; i < SENSOR_COUNT; i++) {
		//printf("thread #%d : %d\n", i, threads[i]);
		if(threads[i]) { 
			// if it has been properly initialized.
			pthread_join(threads[i], NULL);
		}
	}		
	return 0;
}


void* servo_start_routine(void *arg) {
	printf("servo_start_routine executed\n");
	while(true) {
		for(int angle = 160; angle < 250; angle+=5) {	
			ioctl(servo_fd, SERVO_IOCTL_CMD_ROTATE, &angle);
			usleep(75000);	
		}
		for(int angle = 250; angle >= 160; angle-=5) {	
			ioctl(servo_fd, SERVO_IOCTL_CMD_ROTATE, &angle);
			usleep(75000);	
		}
	}
}


void* touch_start_routine(void *arg) {
	printf("touch_start_routine executed\n");
	int count = 0, freq = 0, t = 0;
	while(true) {
			if(count > 3000000) {
				t = 1900;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &t);
				usleep(5000);
				freq = 0;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);
				usleep(5000);
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &t);
				usleep(5000);
				freq = 0;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);
				usleep(5000);
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &t);
				usleep(5000);
				freq = 0;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);
			}

		ioctl(touch_fd, TOUCH_IOCTL_IS_TOUCHED, &isTouched);
		
		if(isTouched == 1) count = 0;
		else count += 1000; 
		usleep(1000);
	}	
}



void* sonar_start_routine(void *arg) {
	printf("sonar_start_routine executed\n");
	unsigned int mm = 0, tmp_mm;
	int DIST_THRESHOLD = 300;

	while(true) {
		for(int i = 0; i < SONAR_SENSOR_COUNT; i++) {
			ioctl(sonar_fd, SONAR_IOCTL_SET_TARGET, &i);
			ioctl(sonar_fd, SONAR_IOCTL_READ_DIST, &mm);
			mm = mm / 5.8;
			sonar_dists[i] = mm;

			if(sonar_dists[0] < DIST_THRESHOLD || sonar_dists[1] < DIST_THRESHOLD) {
				int min_dist = sonar_dists[0] < sonar_dists[1] ? sonar_dists[0] : sonar_dists[1];
				tmp_mm = 2000 - min_dist * 5;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &tmp_mm);
				usleep(100000);

				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);				
			
			}
	
			if(sonar_dists[2] < DIST_THRESHOLD) {
				tmp_mm = 2000 - sonar_dists[2] * 5;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &tmp_mm);
				usleep(150000);
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);
				usleep(150000);
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &tmp_mm);
				usleep(150000);
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);
			}
			
			printf("temperature : %d, light : %d, isMovement : %d, isTouched : %d, SONAR DISTS : [0 : %5umm, 1 : %5umm, 2 : %5umm]\n", temperature, darkness, isSomethingMoved, isTouched, sonar_dists[0], sonar_dists[1], sonar_dists[2]);
			usleep(60000);

		}
		

	}
}

void* infrared_start_routine(void *arg) {
	printf("infrared_start_routine executed\n");	
	int direction = 0;
	
	ioctl(infrared_fd, INFRARED_IOCTL_CMD_SET_DIRECTION, &direction);
	int t = 0;
	int freq = 0;
	while(true) {
		
		ioctl(infrared_fd, INFRARED_IOCTL_CMD_CHECK_MOVEMENT, &isSomethingMoved);


			/*
			if(isSomethingMoved) {
				t = 2000 - 200 * 5;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &t);
				usleep(50000);
				freq = 0;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);
				usleep(50000);
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &t);
				usleep(50000);
				freq = 0;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);
				usleep(50000);
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, &t);
				usleep(50000);
				freq = 0;
				ioctl(buzzer_fd, BUZZER_IOCTL_CMD_PLAY, 0);
			}*/
		usleep(60000);
	}
}
void* pr_start_routine(void *arg) {	
	printf("pr_start_routine executed\n");
	int count;
	int isLEDON = 0;
	count = 0;
	while(true) {
		
		ioctl(pr_fd, PR_IOCTL_CMD_CHECK_LIGHT, &count);		
		if (count > 0) darkness = count;
		
		if(darkness > 170000) {	
			isLEDON = 1;
			printf("LED ON\n");
		} else {
			isLEDON = 0;
			printf("LED OFF\n");
		}
	        ioctl(led_fd, LED_IOCTL_CMD_BLINK, &isLEDON);
		usleep(50000);
	}
}

void* dht11_start_routine(void*) {
	printf("dht11_routine executed\n");
	int temp = 0;
	while(true) {

		ioctl(dht11_fd, DHT11_IOCTL_READ_TEMP , &temp);
		if(temp != -1) {
			temperature = temp; 
			//printf("temp : %d\n", temperature);
		}
		
	}
}
