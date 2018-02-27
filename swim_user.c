#include <stdio.h>  
#include <linux/rtc.h>  
#include <sys/ioctl.h>  
#include <sys/time.h>  
#include <sys/types.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <sys/ioctl.h>

typedef struct APP_WITH_KERNEL{
	unsigned int prescal;
	unsigned int entire_cys;
	unsigned int act_cys;

	unsigned int pulse_state;
	unsigned int pulse_width;
}communication_info_t;


#define A83T_IOCTL_MAGIC 'H'
#define SWIM_IOCTL_RESET 				1
#define A83T_PWM_DUTY_CYCLE_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 0, communication_info_t)
#define A83T_PWM_PULSE_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 1, communication_info_t)



#define DEVICE_NAME "/dev/swim"

int main(int argc, char* argv[]){
	int fd = -1, ret = -1;
	unsigned long val = 0;
	char* result = NULL;
	
	communication_info_t info = {
		.prescal = 0xf,
		.entire_cys = 7,
		.act_cys = 4,

		.pulse_state = 1,
		.pulse_width = 2,
	};
	
	fd = open(DEVICE_NAME, O_RDWR);
	if(fd < 0){
		perror("open /dev/swim failed!\n");
		return -EAGAIN;
	}
	if(argc < 4){
		printf("Use default val: prescal = 0xf, entire_cys = 7, act_cys = 4.\n Usage: ./a.out + prescal + entire_cys + act_cys\n");
	}else{

#if 0
		val = strtoul(argv[1], &result, 16);
		if(val)info.prescal = val;
		printf("input prescal: %#x\n", val);
		
		val = strtoul(argv[2], &result, 16);
		if(val)info.entire_cys = val;
		printf("input entire_cys: %#x\n", val);

		val = strtoul(argv[3], &result, 16);
		if(val)info.act_cys = val;
		printf("input act_cys: %#x\n", val);

#else
		val = strtoul(argv[1], &result, 16);
		if(val)info.pulse_state = val;
		printf("input pulse_state: %#x\n", val);
		
		val = strtoul(argv[2], &result, 16);
		if(val)info.prescal = val;
		printf("input pulse_width: %#x\n", val);

#endif
	}
	ret = ioctl(fd, A83T_PWM_PULSE_IOCTL, (unsigned long)(&info));
	if(!ret){
		perror("ioctl failed!\n");
		return ret;
	}
	
	getchar();
	close(fd);
	return 0;
}
