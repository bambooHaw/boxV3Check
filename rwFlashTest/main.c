#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <strings.h>

#include <sys/select.h>

#include <termios.h>

#include "stm8_swim.h"



#if 0
static void print_buf(char* buf, int len){
	int i;

	for(i=0; i<len; i++){
		#if 0
		if(0== i%10)
			if(0!=i)
				puts("");
		#endif

		printf("0x%02x ", buf[i]);
	}
	puts("");
}
#endif 

static void set_log_info(log_stas_info_t* logp, char* buf, FILE* logfp){
	char buf_tmp[128] = {};
	
	bzero(logp->info, sizeof(logp->info));
	strncat(logp->info, "msg: ", 10);
	if(strlen(buf))
		strncat(logp->info, buf, 64);
	else
		strncat(logp->info, "nothing!", 8);
	
	bzero(buf_tmp, sizeof(buf_tmp));
	snprintf(buf_tmp, 128, " [[Total_cnt(%d), write_cnt(%d), err_cnt(%d), timeout_cnt(%d), errno(%d)(err_try(%d).]]", \
						logp->total_cnt, logp->write_cnt, logp->err_cnt, logp->timeout_cnt, logp->err_num, logp->err_try);
	strncat(logp->info, buf_tmp, 128);
	
	bzero(buf_tmp, sizeof(buf_tmp));
	time(&logp->t);
	//snprintf(buf_tmp, 32, asctime(gmtime(&logp->t)) );
	snprintf(buf_tmp, 32, asctime(localtime(&logp->t)) );
	strncat(logp->info, buf_tmp, 32);
	
	logp->info[sizeof(logp->info)-1] = '\0';
	
	fputs((char*)(&logp->info), logfp);
	fflush(logfp);
}

static void set_uart_speed(int fd, int speed){
	int i, status;
	struct termios opt = {};
	int speed_arry[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300};
	int name_arry[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200, 300};

	tcgetattr(fd, &opt);
	
	for(i=0; i<sizeof(speed_arry)/sizeof(int); i++){
		if(name_arry[i] == speed)
			break;
	}

	tcflush(fd, TCIOFLUSH);
	cfsetispeed(&opt, speed_arry[i]);
	cfsetospeed(&opt, speed_arry[i]);

	opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);	//Input
	opt.c_lflag &= ~OPOST; //Output
	
	status = tcsetattr(fd, TCSANOW, &opt);
	if(0 != status){
		perror("tcsetattr fd");
		return;
	}

	tcflush(fd, TCIOFLUSH);
}



/*设置串口数据位，停止位和校验位
@fd 打开的串口句柄
@databits 数据位，取值位7/8
@stopbits 停止位 取值1/2
@parity 校验位取值N/E/O/S
 */
static int set_uart_parity(int fd, int databits, int stopbits, int parity){
	struct termios options;
	
	if(0 != tcgetattr(fd, &options)){
		perror("Setup serial 1");
		return -1;
	}
	options.c_lflag &= ~CSIZE;

	switch(databits){
		case 7:
			options.c_lflag |= CS7;
			break;
		case 8:
			options.c_lflag |= CS8;
			break;
		default:
			fprintf(stderr, "Unsupported data size!\n");
			return -1;
	}


	switch(stopbits){
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr, "Unsupported stop bits!\n");
			return -1;
	}

	switch(parity){
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB; //关闭使能
			options.c_iflag &= ~INPCK; //使能 parity检查
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB); //设置位奇校验
			options.c_iflag |= INPCK;
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB; //使能校验
			options.c_cflag &= ~PARODD; //转换位偶校验
			options.c_iflag |= INPCK;
			break;
		case 's':
		case 'S':
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
		default:
			fprintf(stderr, "Unsupported parity!\n");
			return -1;
	}


	/*设置优先选项*/
	if(parity != 'n')
		options.c_iflag |= INPCK;

	tcflush(fd, TCIOFLUSH);
	options.c_cc[VTIME] = 150; //超时设置为5s
	options.c_cc[VMIN] = 0; //立即更新设置
	if(0 != tcsetattr(fd, TCSANOW, &options)){
		perror("Setup serial 3");
		return -1;
	}

	return 0;
}


int main(int argc, char* argv[]){
	int write_flash_cnt = 0, cnt = 0, ret = -1;
	int uart_fd = -1, swim_fd = -1;
	log_stas_info_t log = {};
	FILE* logfp = NULL, *bootfp = NULL;
	char buf[8192] = {};
	fd_set rd_fdset;
	struct timeval tv; //delay time in select()

	//1. open or create log file
	logfp = fopen(STM8_LOG_PATH, "a");
	if(!logfp){
		perror("open log file failed!\n");
		goto open_log_err;
	}else{
		bzero(&log, sizeof(log_stas_info_t));
	}

	//2.0 open uart
	uart_fd = open(STM8_UART_PATH, O_RDWR);
	if(uart_fd<0){
		perror("open uart2 failed!\n");
		log.err_num = errno;
		set_log_info(&log, "open log failed!", logfp);
		fputs((char*)(&log.info), logfp);
		goto open_uart_err;
	}else{
	//2.1 set uart
		set_uart_speed(uart_fd, 115200);
		if(-1 == set_uart_parity(uart_fd, 8, 1, 'N')){
			printf("Set parity error!\n");
			goto set_uart_err;
		}
	}

	write_flash_cnt = 1000;
	while(write_flash_cnt--){
		//3. open /dev/swim
		log.err_try = 0;
	open_stm8_swim_loop:
		swim_fd = open(STM8_SWIM_PATH, O_RDWR);
		if(swim_fd<0){
			perror("open swim failed!\n");
			log.err_cnt++;
			log.err_try++;
			log.err_num = errno;
			set_log_info(&log, "open swim failed!", logfp);
			if(log.err_try>SWIM_MAX_ERR_TRY)
				goto open_swim_err;
			else
				goto open_stm8_swim_loop;
		}
		// 4. open ***_boot.bin file
		if(argc<2){
			bootfp = fopen(STM8_BOOT_PATH, "r");
		}else{
			bootfp = fopen(argv[1], "r");
		}
		
		if(!bootfp){
			perror("open boot file failed!\n");
			log.err_num = errno;
			set_log_info(&log, "open boot failed!", logfp);
			goto open_boot_err;
		}
		cnt = 0;
		bzero(buf, sizeof(buf));
		while(!feof(bootfp)){
			buf[cnt++] = fgetc(bootfp);
		}
		//update log
		log.total_cnt = cnt;
		//printf("%s's size is %d bytes(%d pages(64bytes/page)).\n", STM8_BOOT_PATH, cnt, (cnt/64+((cnt%64)?1:0)) );

		log.err_try = 0;
write_stm8_swim_loop:
		//printf("Begin to writting flash...\n");
		ret = write(swim_fd, buf, sizeof(buf));
		if(ret<0){
			perror("write failed!\n");
			log.err_num = errno;
			log.err_try++;
			set_log_info(&log, "write failed!", logfp);
			if(log.err_try>SWIM_MAX_ERR_TRY)
				goto write_flash_err;
			else
				goto write_stm8_swim_loop;
		}

		//update log
		log.write_cnt++;
		log.err_num = 0;
		set_log_info(&log, "write stm8flash success.", logfp);
		
		//printf("STM8Flash has been written success!\n");


		fclose(bootfp);
		log.err_try = 0;
close_stm8_swim_loop:
		//printf("Rst & Boot stm8flash...\n");
		ret = close(swim_fd);
		if(ret<0){
			perror("close failed!\n");
			log.err_num = errno;
			log.err_try++;
			set_log_info(&log, "close failed!", logfp);
			if(log.err_try>SWIM_MAX_ERR_TRY)
				;//do nothing.
			else
				goto close_stm8_swim_loop;
			
		}
		
		sleep(3); //wait for stm8 rst and boot success.
		
		// 5. select the arm's uart2 for read ack from stm8 uart
		FD_ZERO(&rd_fdset);
		FD_SET(uart_fd, &rd_fdset);
		tv.tv_sec = 5;//timeout detect is 5s
		tv.tv_usec = 0;
		ret = select(uart_fd+1, &rd_fdset, NULL, NULL, &tv);
		if(0==ret){
			//update log
			log.timeout_cnt++;
			perror("wait for stm8 ack timeout\n");
			log.err_num = -20;
			set_log_info(&log, "Wait timeout!", logfp);
			continue;
		}
		if(0 > ret){
			printf("select(%s) return %d. [%d]:%s\n", STM8_UART_PATH, ret, errno, strerror(errno));
			continue;
		}
		if(FD_ISSET(uart_fd, &rd_fdset)){
			bzero(log.buf, sizeof(log.buf));
			ret = read(uart_fd, log.buf, sizeof(log.buf));
			buf[sizeof(log.buf) - 1] = '\0';   //protocal length is 11
			//printf("Stm8(read from %s)(%d): ", STM8_UART_PATH, ret);
			//print_buf(log.buf, ret);
			//update log
			log.err_num = 0;
			set_log_info(&log, log.buf, logfp);
		}
	}
	//end
	close(uart_fd);
	fclose(logfp);
	return 0;
	
write_flash_err:
	fclose(bootfp);
open_boot_err:
	close(swim_fd);
open_swim_err:
set_uart_err:
	close(uart_fd);
open_uart_err:
	fclose(logfp);
open_log_err:
	return -errno;
}
