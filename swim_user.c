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
	unsigned int pwm_ch_ctrl;
	unsigned int prescal;
	unsigned int entire_cys;
	unsigned int act_cys;

	unsigned int pulse_state;
	unsigned int pulse_width;

	unsigned int bit;
	
	unsigned int addr;
	unsigned char buf[128];
	unsigned int count;
}communication_info_t;


#define A83T_IOCTL_MAGIC 'H'
#define SWIM_IOCTL_RESET 				1
#define A83T_PWM_DUTY_CYCLE_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 0, communication_info_t)
#define A83T_PWM_PULSE_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 1, communication_info_t)
#define A83T_PWM_REG_CTRL_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 2, communication_info_t)
#define A83T_PWM_REG_PERIOD_IOCTL		_IOWR(A83T_IOCTL_MAGIC, 3, communication_info_t)
#define A83T_SWIM_READ_IOCTL		_IOR(A83T_IOCTL_MAGIC, 4, communication_info_t)
#define A83T_SWIM_WRITE_IOCTL		_IOW(A83T_IOCTL_MAGIC, 5, communication_info_t)



#define DEVICE_NAME "/dev/swim"

int main(int argc, char* argv[]){
	int fd = -1, ret = -1;
	unsigned long val = 0;
	char* result = NULL;
	
	communication_info_t info = {
		.pwm_ch_ctrl = 0x0,
		.prescal = 0xf,
		.entire_cys = 7,
		.act_cys = 4,

		.pulse_state = 1,
		.pulse_width = 2,

		.count = 1,
	};
	
	fd = open(DEVICE_NAME, O_RDWR);
	if(fd < 0){
		perror("open /dev/swim failed!\n");
		return -EAGAIN;
	}

	#if 1
	if(argc < 2){
		printf("Use default val: pwm_ch_ctrl = 0x0, entire_cys = 7, act_cys = 4.\n Usage: ./a.out + 1/2 + ctl/period\n");
		return -1;
	}else{
		val = strtoul(argv[1], &result, 16);	//0 -> read
		if(0 == val){
			val = strtoul(argv[2], &result, 16);
			info.addr = val;
			printf("read addr: %#x(%d)\n", info.addr, info.addr);
					
			ret = ioctl(fd, A83T_SWIM_READ_IOCTL, (unsigned long)(&info));
			if(ret){
				perror("ioctl failed!\n");
				return ret;
			}
		}else if(1 == val){	//1 -> WRITE
			val = strtoul(argv[2], &result, 16);
			info.addr = val;
			val = strtoul(argv[3], &result, 16);
			info.buf[0] = (unsigned char)val;
			
			printf("write addr: %#x(%d), buf:%s\n", info.addr, info.addr, info.buf);
	
			ret = ioctl(fd, A83T_SWIM_WRITE_IOCTL, (unsigned long)(&info));
			if(ret){
				perror("ioctl failed!\n");
				return ret;
			}
			sleep(1);
			ret = ioctl(fd, A83T_SWIM_READ_IOCTL, (unsigned long)(&info));
			if(ret){
				perror("ioctl failed!\n");
				return ret;
			}

		}

	}
	#endif 

	
	getchar();
	close(fd);
	return 0;
}
