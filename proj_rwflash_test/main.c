#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <strings.h>

#include <sys/select.h>

#include "stm8_swim.h"

static void set_log_info(log_stas_info_t* logp, char* buf, FILE* logfp){
	bzero(logp->info, sizeof(logp->info));
	time(&logp->t);
	snprintf(logp->info, 32, "%s", asctime(gmtime(&logp->t)));
	snprintf(logp->info, 72, ": total_cnt(%8d), write_cnt(%8d), timeout_cnt(%8d).", logp->total_cnt, logp->write_cnt, logp->timeout_cnt);
	snprintf(logp->info, 24, "errno(%8d)", logp->err_num);
	if(buf)
		snprintf(logp->info, 32, "%s", buf);
	logp->info[sizeof(logp->info)-1] = '\0';
	fputs((char*)(&logp->info), logfp);
	fflush(logfp);
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

	//2. open uart
	uart_fd = open(STM8_UART_PATH, O_RDWR);
	if(uart_fd<0){
		perror("open uart2 failed!\n");
		log.err_num = errno;
		set_log_info(&log, "open log failed!", logfp);
		fputs((char*)(&log.info), logfp);
		goto open_uart_err;
	}

	write_flash_cnt = 1000;
	while(write_flash_cnt--){
		//3. open /dev/swim
		swim_fd = open(STM8_SWIM_PATH, O_RDWR);
		if(swim_fd<0){
			perror("open swim failed!\n");
			log.err_num = errno;
			set_log_info(&log, "open swim failed!", logfp);
			goto open_swim_err;
		}
		// 4. open ***_boot.bin file
		bootfp = fopen(STM8_BOOT_PATH, "r");
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
		printf("%s's size is %d bytes(%d pages(64bytes/page)).", STM8_BOOT_PATH, cnt, (cnt/64+((cnt%64)?1:0)) );

		ret = write(swim_fd, buf, sizeof(buf));
		if(ret<0){
			perror("write failed!\n");
			log.err_num = errno;
			set_log_info(&log, "write failed!", logfp);
			goto write_flash_err;
		}

		//update log
		log.write_cnt++;
		log.err_num = 0;
		set_log_info(&log, "write stm8flash success.", logfp);


		fclose(bootfp);
		close(swim_fd);
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
			bzero(buf, sizeof(buf));
			ret = read(uart_fd, buf, sizeof(buf));
			buf[12] = '\0';   //protocal length is 11
			printf("read(%s) return(%d).\n", STM8_UART_PATH, ret);
			//update log
			log.err_num = 0;
			set_log_info(&log, "Stm8 ack success.", logfp);
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
	close(uart_fd);
open_uart_err:
	fclose(logfp);
open_log_err:
	return -errno;
}
