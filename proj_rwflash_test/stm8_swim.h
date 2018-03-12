#ifndef __STM8_SWIM_H__
#define __STM8_SWIM_H__

#include <time.h>


#define STM8_LOG_PATH "/home/log/log_swim_st8.txt"
#define STM8_UART_PATH "/dev/ttyS2"
#define STM8_SWIM_PATH "/dev/swim"
#define STM8_BOOT_PATH "4444.bin"


typedef struct LOG_STATISTICS_INFO{
#define SWIM_MAX_ERR_TRY 10
	time_t t;
	int err_num;
	int write_cnt;
	int timeout_cnt;
	int err_cnt;
	int err_try;
	int total_cnt;

	char buf[32];
	char info[256];
}log_stas_info_t;



#endif
