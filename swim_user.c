#include <stdio.h>  
#include <linux/rtc.h>  
#include <sys/ioctl.h>  
#include <sys/time.h>  
#include <sys/types.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <errno.h>  

#define DEVICE_NAME "/dev/swim"

int main(int argc, char* argv[]){
	int fd = -1;

	while(1){
		fd = open(DEVICE_NAME, O_RDWR);
		if(fd < 0){
			perror("open /dev/swim failed!\n");
			return -EAGAIN;
		}
		getchar();
		close(fd);
	}
	return 0;
}
