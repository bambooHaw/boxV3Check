#include <stdio.h>  
#include <linux/rtc.h>  
#include <sys/ioctl.h>  
#include <sys/time.h>  
#include <sys/types.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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

//int = strtoul(argv[1], &result, 16);	//0 -> read

int main(int argc, char* argv[]){
	int fd = -1, ret = -1, cnt =0;
	char buf[8192] = {};
	FILE* fp = NULL;
	
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
	if(argc < 2){
		printf("Usage: ./stm8boot + st8bootfilename.\n");

	}else{
		fp = fopen(argv[1], "r");
		bzero(buf, 8192);
		while(!feof(fp)){
			buf[cnt++] = fgetc(fp);
		}
		#if 0
		printf("\n");
		printf("\n");
		printf("%s's size is %d bytes.\n", argv[1], cnt);
		printf("Begin to writting...\n");
		#endif
		
		ret = write(fd, buf, cnt);
		if(ret < 0){
			perror("Write stm8 failed!\n");
		}else{
			;//printf("stm8 flash has been written successfully.\n");
		}	
		
		fclose(fp);
	}


	close(fd);
	return 0;
}
