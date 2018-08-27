#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include<time.h>  

#define STM8_CMD_MIN_LENGTH	9
#define STM8_CMD_AND_DATA_LENGTH_MAX 42
#define NODE_PATH_UART_STM8  "/dev/ttyS2"
#define NODE_PATH_UART_232	"/dev/ttyS3"

#define CIRCLE_CNT	12

#define hensen_debug(string, args...)\
	do{\
		printf("%s,%s()%u---"string, __FILE__, __func__, __LINE__, ##args);\
		puts("");\
	}while(0)

typedef struct _UART232_TRANS_RANDOM_MSG{
	char head[3];
	int random;
	char end[2];
}random_msg_t;




/*	设置串口通信速率
 *	@fd 打开的串口句柄
 @speed 串口速度
 *	返回值：void
 */
void setUartSpeed(int fd, int speed){
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
		hensen_debug("tcsetattr fd");
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
int setUartParity(int fd, int databits, int stopbits, int parity){
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


void str_print(char* buf, int len){
	int i;

	for(i=0; i<len; i++){
		if(0== i%10)
			if(0!=i)
				puts("");

		printf("0x%02x ", buf[i]);
	}
	puts("");
}

char calcStm8CmdParity(char* cmdBuf, int bufLen)
{
	char ret = 0;
	char unparity = 0;
	int i=0;

	if((!cmdBuf) || (bufLen < STM8_CMD_MIN_LENGTH))
	{
		ret = -EINVAL;
		return ret;
	}

	for(i=0; i<bufLen; i++)  unparity += cmdBuf[i];

	cmdBuf[bufLen - 1 - 2] = 0 - unparity;
}

char getStm8CmdBuf(char* cmdBuf, int* bufLen, char rdwr, char mainCmd, char subCmd, char dataLen, char *data)
{
	char ret = 0;
	int fd = -1;

	if(!bufLen)
		return -EINVAL;

	*bufLen = STM8_CMD_MIN_LENGTH + dataLen;
	cmdBuf = malloc(*bufLen);
	if(!cmdBuf)
	{
		ret = -ENOMEM;
		printf("Warning: malloc cmdBuf for stm8 failed!\n");
		return ret;
	}

	cmdBuf[0] = 0x55;
	cmdBuf[1] = 0xaa;
	cmdBuf[2] = rdwr;
	cmdBuf[3] = mainCmd;
	cmdBuf[4] = subCmd;
	cmdBuf[5] = dataLen;
	if(rdwr)//write
	{
		if(dataLen>0)
		{
			//if(strlen(data) < dataLen) dataLen = strlen(data);
			memcpy(cmdBuf + 6, data, dataLen);
		}
	}
	cmdBuf[6 + dataLen + 0] = 0x00;//clear the parity
	cmdBuf[6 + dataLen + 1] = 0xf0;
	cmdBuf[6 + dataLen + 2] = 0x0f;

	calcStm8CmdParity(cmdBuf, *bufLen);

	return ret;
}

int uartCommunicateWithStm8(char* buf, int len)
{
	int ret = 0;
	int fd = -1;
	fd_set rdFdset;
	int timeoutCnt = 0;
	struct timeval timer;
	bzero(&timer, sizeof(struct timeval));

	fd = open(NODE_PATH_UART_STM8, O_RDWR);
	if(fd<0)
	{
		ret = -EAGAIN;
		printf("Can't open '%s'\n", NODE_PATH_UART_STM8);
		return ret;
	}else
	{
		setUartSpeed(fd, 115200);
		if(-1 == setUartParity(fd, 8, 1, 'N'))
		{
			ret = -EIO;
			("Set parity error!\n");
			close(fd);
			return ret;
		}
	}

	/*
	 * uart2 write() check cmd buf to stm8, select() stm8's read() in 3s,
	 */
	/* write once, write check cmd buf to uart485
	 * 9 is the cmd length , Please do not change it rashly
	 */
	ret = write(fd, buf, len);

	while(1)
	{
		timer.tv_sec = 3;
		timer.tv_usec = 0;
		FD_ZERO(&rdFdset);
		FD_SET(fd, &rdFdset);
		ret = select(fd+1, &rdFdset, NULL, NULL, &timer);
		if(0 == ret)
		{
			timeoutCnt++;
			if(2 >= timeoutCnt)
			{
				ret = write(fd, buf, len);
			}else
			{
				printf("Can't read.");
				close(fd);
				return -5;
			}
		}
		if(0>ret){
			printf("select(%s) return %d. [%d]:%s\n", NODE_PATH_UART_STM8, ret, errno, strerror(errno));
			close(fd);
			return -4;
		}else
		{
			if(FD_ISSET(fd, &rdFdset)){
				bzero(buf, len);
				ret = read(fd, buf, len);
				close(fd);
				return 0;
			}
		}
	}
	//test 007 end
	close(fd);
	return -6;
}

int getRealRandom(void)
{
	int selfRandom = 0;

	srand((int)time(0));

	/*1.0
	 * [a,b) -> (rand()%(b-a)) + a
	 * [a,b] -> (rand()%(b-a+1)) + a
	 */

	/*2.0
	 * selfRandom = 1+(int)(50.0*rand()/(RAND_MAX+1.0));
	 */
	selfRandom = (rand()%(255-2)) + 2;

	return selfRandom;
}

int tryEstablishCommunicationWithUart232(int times, int interval)
{
	int ret = 0, tryCnt = 0, waitCnt = 0, readCnt = 0, j=0;
	int fd = -1;
	fd_set rd_fdset;
	struct timeval dly_tm; //delay time in select()
	char buf[32] = {};

	if(0 >= times) times = 1;

	for(waitCnt; waitCnt<times; waitCnt++)
	{
		fd = open(NODE_PATH_UART_232, O_RDWR);
		if(fd<0)
		{
			printf("%s open failed!\n", NODE_PATH_UART_232);
			ret = -ENODEV;
			return ret;
		}


		setUartSpeed(fd, 115200);
		if(-1 == setUartParity(fd, 8, 1, 'N'))
		{
			printf("Set parity error!\n");
			ret = -ENXIO;
		}

		while(1)
		{
			FD_ZERO(&rd_fdset);
			FD_SET(fd, &rd_fdset);
			if(interval <= 0) interval = 1;
			dly_tm.tv_sec = interval;
			dly_tm.tv_usec = 0;
			ret = select(fd+1, &rd_fdset, NULL, NULL, &dly_tm);
			if(0==ret)
			{
				if(waitCnt++ > times) break;

				ret = -EAGAIN;
				printf("wait(%d) to establish communication again...\n", waitCnt);
				continue;
			}
			else if(0 > ret)
			{
				tryCnt++;
				printf("select return %d. [%d]:%s\n", ret, errno, strerror(errno));
				printf("try(%d) to establish communication again...\n", tryCnt);
				sleep(1);
				break;
			}else
			{

		
				if(FD_ISSET(fd, &rd_fdset))
				{
					readCnt++;
					bzero(buf, sizeof(buf));
					read(fd, buf, sizeof(buf));
					for(j=0; j<sizeof(buf); j++)
					{
						if(0 == strncmp("v", buf+j, 1))
						{
							//communicate success;
							ret = 0;
							break;
						}else
						{
							ret = -ENXIO;
							printf("read(%d) to establish communication again...\n", readCnt);
						}
					}
				}

			}
		}
		close(fd);
		if(!ret)
		{
			//success
			break;
		}
	}
	return ret;

}

int getMsgHead(int fd)
{
	int ret = -EAGAIN;

	char tmpCh = 0;
	read(fd, &tmpCh, 1);
	if('r' == tmpCh)
	{
		read(fd, &tmpCh, 1);
		if('d' == tmpCh)
		{
			read(fd, &tmpCh, 1);
			if('m' == tmpCh)
			{
				ret = 0;
			}
		}
	}

	return ret;
}
//2. uart232 to coordinate random(must be different from another box)
int coordinateRandomFromUart232(int* randomOwn, int* randomOth, int* mainFlag)
{
	int ret = 0, readCnt = 0, i = 0, successDelayCnt = 0;
	int fd = -1;
	fd_set rd_fdset;
	random_msg_t random_info[2];
	struct timeval dly_tm; //delay time in select()

	fd = open(NODE_PATH_UART_232, O_RDWR);
	if(fd<0)
	{
		printf("%s open failed!\n", NODE_PATH_UART_232);
		ret = -ENODEV;
	}

	setUartSpeed(fd, 115200);
	if(-1 == setUartParity(fd, 8, 1, 'N')){
		printf("Set parity error!\n");
		ret = -EIO;
	}


	//init random msg own and other
	*randomOwn = getRealRandom();
	memcpy(random_info[0].head, "rdm", 3);
	random_info[0].random = *randomOwn;
	memcpy(random_info[0].end, "tt", 2);

	while(1)
	{
		write(fd, &random_info[0], sizeof(random_msg_t));

		if(0 != successDelayCnt)
		{
			if(successDelayCnt > 10)
			{
				break;
			}else
			{
				successDelayCnt++;
				sleep(1);
				//rewrite for other get our own random
				continue;
			}
		}


		FD_ZERO(&rd_fdset);
		FD_SET(fd, &rd_fdset);
		dly_tm.tv_sec = 2;
		dly_tm.tv_usec = 0;
		ret = select(fd+1, &rd_fdset, NULL, NULL, &dly_tm);
		if(0==ret)
		{
			ret = -EAGAIN;
			hensen_debug("select() return %d, fd=%d", ret, fd);
			break;
		}
		if(0 > ret){
			ret = -EBUSY;
			printf("select return %d. [%d]:%s\n", ret, errno, strerror(errno));
			break;
		}else
		{

			if(FD_ISSET(fd, &rd_fdset))
			{
				if(0==successDelayCnt)
				{
					readCnt++;
					bzero(&random_info[1], sizeof(random_msg_t));
					if(!getMsgHead(fd))
					{
						//success
						ret = 0;
						read(fd, (char*)(&random_info[1]) + 3, sizeof(random_msg_t) - 3);
						*randomOth = random_info[1].random;
						successDelayCnt++;
						continue;
					}
					else
					{
						//read again
						ret = -EAGAIN;
						if(readCnt > 1024) break;
					}
				}
			}else
			{
				ret = -EAGAIN;
			}
		}
	}
	close(fd);


	//compare two random
	if(*randomOwn <2 || *randomOwn > 254 || *randomOth <2 || *randomOth > 254)
	{
		*mainFlag = -EINVAL;
		ret = -ENXIO;
		printf("Error: Wrong random.\n");
	}else
	{
		if(*randomOwn > *randomOth)
		{
			*mainFlag = 0;
		}else if(*randomOwn < *randomOth)
		{
			*mainFlag = 1;
		}else
		{
			*mainFlag = -EAGAIN;
			ret = -EAGAIN;
		}
	}

	printf("randomOwn:%d, randomOther:%d, mainFlag:%d.\n", *randomOwn, *randomOth, *mainFlag);
	return ret;
}

void createEnv(void)
{
	system("rm -rf /tmp/aging/");
	system("mkdir -p /tmp/aging/");
}

static char *itoa(int value, char *string, int radix)
{
	char stack[16];
	int  negative = 0;			//defualt is positive value
	int  i;
	int  j;
	char digit_string[] = "0123456789ABCDEF";	
	
	if(value == 0)
	{
		//zero
		string[0] = '0';
		string[1] = '\0';
		return string;
	}
	
	if(value < 0)
	{
		//'value' is negative, convert to postive first
		negative = 1;
		value = -value ;
	}
	
	for(i = 0; value > 0; ++i)
	{
		// characters in reverse order are put in 'stack'.
		stack[i] = digit_string[value % radix];
		value /= radix;
	}
	
	//restore reversed order result to user string
    j = 0;
	if(negative)
	{
		//add sign at first charset.
		string[j++] = '-';
	}
	for(--i; i >= 0; --i, ++j)
	{
		string[j] = stack[i];
	}
	//must end with '\0'.
	string[j] = '\0';
	
	return string;
}

int main(int argc, char* argv[])
{
	int ret = 0, cCnt = 0;
	int randomOwn = 0, randomOther = 0, mainFlag = 0;
	char wanIp[2][16] = {};
	char lanIp[2][16] = {};
	char tmpIpNum[3] = {};
	char cmdStr[64] = {};

	printf("Debug: %s(%d).\n", __func__, __LINE__);
	createEnv();
#if 0
	if(tryEstablishCommunicationWithUart232(7, 1))
	{
		printf("Error: tryEstablishCommunicationWithUart232 failed!\n");	
	}
#endif

	
	sleep(3);
	//for(cCnt=0; cCnt<CIRCLE_CNT; cCnt++)
	while(1)
	{
		printf("Debug: ret:%d, randomOwn:%d, randomOther:%d, mainFlag:%d.\n", ret, randomOwn, randomOther, mainFlag);
		ret = coordinateRandomFromUart232(&randomOwn, &randomOther, &mainFlag);
		if(ret)
		{
			sleep(1);
		}else
		{
			if(0 == mainFlag)
			{
				system("echo " " > /tmp/aging/host");
				break;
			}else if(1 == mainFlag)
			{
				system("echo 1 > /tmp/aging/host");	
				break;
			}else
			{
				continue;
			}
		}
	}

	if(ret)
	{
		//failed
		ret = -EAGAIN;
		printf("Error: coordinate with another uart for a random failed!\n");
	}else
	{
		if((0 == mainFlag) || (1 == mainFlag))
		{
			//own
			strncpy(wanIp[0], "192.168.1.", 10);
			strncpy(lanIp[0], "192.168.2.", 10);
			//other
			strncpy(wanIp[1], "192.168.1.", 10);
			strncpy(lanIp[1], "192.168.2.", 10);

			bzero(tmpIpNum, sizeof(tmpIpNum));
			itoa(randomOwn, tmpIpNum, 10);
			strcat(wanIp[0], tmpIpNum);
			strcat(lanIp[0], tmpIpNum);
			bzero(tmpIpNum, sizeof(tmpIpNum));
			itoa(randomOther, tmpIpNum, 10);
			strcat(wanIp[1], tmpIpNum);
			strcat(lanIp[1], tmpIpNum);

			bzero(cmdStr, sizeof(cmdStr));
			strcat(cmdStr, "ifconfig eth0 ");
			strcat(cmdStr, wanIp[0]);
			system(cmdStr);
			bzero(cmdStr, sizeof(cmdStr));
			strcat(cmdStr, "ifconfig eth1 ");
			strcat(cmdStr, lanIp[0]);
			system(cmdStr);

			bzero(cmdStr, sizeof(cmdStr));
			strcat(cmdStr, "echo ");
			strcat(cmdStr, wanIp[1]);
			strcat(cmdStr, " > /tmp/aging/wan");
			system(cmdStr);
			bzero(cmdStr, sizeof(cmdStr));
			strcat(cmdStr, "echo ");
			strcat(cmdStr, lanIp[1]);
			strcat(cmdStr, " > /tmp/aging/lan");
			system(cmdStr);
			//
			system("killall -9 /opt/ethaging.sh");
			system("chmod 777 /opt/ethaging.sh");
			system("/opt/ethaging.sh &");
		}else
		{
			;//failed
		}
	}

	return ret;
}
